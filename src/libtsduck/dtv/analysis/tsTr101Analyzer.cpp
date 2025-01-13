//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Staz Modrzynski
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Transport stream analyzer.
//
//----------------------------------------------------------------------------


#include "tsTr101Analyzer.h"

#include <functional>
#include <tsBinaryTable.h>
#include <tsPAT.h>
#include <tsPMT.h>
#include <tsSectionDemux.h>

ts::TR101_290Analyzer::TR101_290Analyzer(TSP* tsp) :
    _tsp(tsp), _demux()
{
    auto ptr = std::make_shared<ts::TR101_290Analyzer::ServiceContext>(0, ServiceContext::ServiceContextType::Pat);
    _services.insert(std::make_pair(0, ptr));
}

void ts::TR101_290Analyzer::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    auto service = getService(table.sourcePID());

    if (table.sourcePID() == PID_PAT && table.tableId() != TID_PAT) {
        service->pat_error++;
        service->pat_error_2++;
    }

    if (service->_type == ServiceContext::Pmt && table.tableId() != TID_PMT) {
        service->pmt_error++;
        service->pmt_error_2++;
    }

    if (table.tableId() == TID_PAT && service->_type == ServiceContext::Pat) {
        service->_type = ServiceContext::Pat;

        const auto pat = PAT(_duck, table);

        // Assign PMTs.
        for (auto [service_id, pid] : pat.pmts) {
            auto s2 = getService(pid);
            s2->_type = ServiceContext::Pmt;
            s2->_pmt_service_id = service_id;
        }

        // Remove PMT assignments.
        for (auto it = _services.begin(); it != _services.end(); ++it) {
            if (it->second->_type == ServiceContext::Pmt && !pat.pmts.contains(it->second->_pmt_service_id)) {
                it->second->_type = ServiceContext::Unassigned;
            }
        }

    } else if (table.tableId() == TID_PMT && service->_type == ServiceContext::Pmt) {
        const auto pmt = PMT(_duck, table);

        // Ensure all PIDs are assigned to this service.
        for (auto map : pmt.streams) {
            const auto s2 = getService(map.first);
            s2->_type = ServiceContext::Assigned;
            s2->_pmt_service_id = service->_pmt_service_id;
        }

        // Remove PIDs that are Assigned to this service ID.
        for (auto it = _services.begin(); it != _services.end(); ++it) {
            if (it->second->_type == ServiceContext::Assigned && it->second->_pmt_service_id == service->_pmt_service_id) {
                if (!pmt.streams.contains(it->first)) {
                    it->second->_type = ServiceContext::Unassigned;
                }
            }
        }
    } else if (table.tableId() == TID_CAT) {
        // todo: figure out a timeout for this so this isn't true forever.
        _has_cat = true;
    }
}

std::shared_ptr<ts::TR101_290Analyzer::ServiceContext> ts::TR101_290Analyzer::getService(PID pid)
{
    auto it = _services.find(pid);
    if (it == _services.end()) {
        auto service = std::make_shared<ServiceContext>(ServiceContext{
            ._pid = pid,
            ._type = ServiceContext::Unassigned
        });
        _services.insert(std::make_pair(pid, service));
        return service;
    }
    return it->second;
}

void ts::TR101_290Analyzer::processPacket(ServiceContext& ctx, const TSPacket& pkt, const TSPacketMetadata& mdata, const uint64_t bitrate) const
{

    // todo: do TS packets ever arrive with invalid sync bytes this deep in?
    if (!pkt.hasValidSync()) {
        ctx.sync_byte_error++;
        return;
    }

    if (pkt.getTEI()) {
        ctx.transport_error++;
    }

    if (pkt.isScrambled() && !_has_cat) {
        ctx.cat_error++;
    }

    if (ctx._type == ServiceContext::Pat) {
        if (pkt.getScrambling()) {
            ctx.pat_error++;
            ctx.pat_error_2++;
        }
    } else if (ctx._type == ServiceContext::Pmt) {
        if (pkt.getScrambling()) {
            ctx.pmt_error++;
            ctx.pmt_error_2++;
        }
    }

    const auto pkt_ts = mdata.getInputTimeStamp().count();

    if (ctx.first_packet) {
        ctx.last_cc = pkt.getCC();
        ctx.last_packet_ts = pkt_ts;
        ctx.first_packet = false;

        if (pkt.hasPTS()) {
            ctx.last_pts_ts = pkt_ts;
        }

        if (pkt.hasPCR()) {
            ctx.last_pcr_val = pkt.getPCR();
            ctx.last_pcr_ts = pkt_ts;
            ctx.has_pcr = true;
        }

        return;
    }

    // Validate CC errors.
    auto expected_cc = ctx.last_cc;
    if (pkt.hasPayload()) {
        expected_cc = (expected_cc + 1) % 16;
    }

    if (!pkt.getDiscontinuityIndicator() && expected_cc != pkt.getCC()) {
        ctx.cc_error++;
    }

    // PID error.
    // todo: do we want to be doing PID error tests on PAT/PMT as well?
    if (bitrate && ctx._type != ServiceContext::Unassigned) {
        const auto dur = (pkt_ts - ctx.last_packet_ts) / bitrate;
        if (dur > 5 * SYSTEM_CLOCK_FREQ) {
            ctx.pid_error++;
        }
    }

    // PTS error.
    if (pkt.hasPTS()) {
        if (bitrate) {
            if (ctx.last_pts_ts != INVALID_PTS) {
                auto dur = (pkt_ts - ctx.last_pts_ts) / bitrate;
                if (dur > 700 * SYSTEM_CLOCK_FREQ /  1000) {
                    ctx.ptr_error++;
                }
            }
        }
        ctx.last_pts_ts = pkt_ts;
    }

    // PCR error.
    if (pkt.hasPCR()) {
        auto pcr = pkt.getPCR();
        if (bitrate && ctx.last_pcr_val != INVALID_PCR && ctx.last_pcr_ts != INVALID_PCR) {
            auto pcr_dist = pcr - ctx.last_pcr_val;
            auto pcr_dist_dur = pcr_dist / bitrate;
            if (llabs(pcr_dist) > 100 * SYSTEM_CLOCK_FREQ /  1000) {
                ctx.pcr_discontinuity_indicator_error++;
                ctx.pcr_error++;
            }

            auto pkt_dist = pkt_ts - ctx.last_pcr_ts;
            auto pkt_dist_dur = pkt_dist / bitrate;
            if (pkt_dist_dur > 100 * SYSTEM_CLOCK_FREQ /  1000) {
                ctx.pcr_repetition_error++;
                ctx.pat_error++;
            }

            auto delta = llabs(pcr_dist_dur - pkt_dist_dur);
            if (delta > 500 * SYSTEM_CLOCK_FREQ / 1e9) {
                ctx.pcr_accuracy_error++;
            }
        }

        ctx.last_pcr_val = pcr;
        ctx.last_pcr_ts = pkt_ts;
    }

    ctx.last_packet_ts = pkt_ts;
    ctx.last_cc = pkt.getCC();
}

void ts::TR101_290Analyzer::feedPacket(const TSPacket& packet, const TSPacketMetadata& mdata)
{
    auto service = getService(packet.getPID());
    auto bitrate = _tsp->bitrate();
    processPacket(*service, packet, mdata, _tsp->bitrate().toInt64());

    if (packet.getPID() == PID_PAT) {
        if (packet.getPUSI()) {

        }
    }
}

const char* ERR = "[ERR]";
const char* OK  = "[OK] ";
const char* NA  = "[N/A]";
struct ErrorState {
    int count;
    bool show;
};

typedef std::function<ErrorState(const ts::TR101_290Analyzer::ServiceContext&)> get_func;


static int count(const get_func& fn, std::map<int,  std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
{
    int count = 0;
    for (auto it = _services.begin(); it != _services.end(); it++) {
        auto val = fn(*it->second);
        if (val.show)
            count += val.count;
    }
    return count;
}

static void print(const char* name, const get_func& fn,  std::ostream& stm, std::map<int, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
{
    for (auto it = _services.begin(); it != _services.end(); it++) {
        auto val = fn(*it->second);
        if (val.show)
            stm << "\t" << (val.count == 0 ? OK : ERR) << "PID 0x" << std::format("{:X}", it->second->_pid) << ": " << val.count << "\n";
    }
}

static void print_real(const char* name, const get_func& fn, std::ostream& stm, std::map<int, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
{
    int count = ::count(fn , _services);

    stm << (count == 0 ? OK : ERR) << " " << name << ": " << count << "\n";
    print(name, fn, stm, _services);
}

static void print_custom(const char* name, int count, std::ostream& stm, std::map<int, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
{
    stm << (count == 0 ? OK : ERR) << " " << name << ": " << count << "\n";
}

bool is_pes(const ts::TR101_290Analyzer::ServiceContext& ctx) {return ctx._type == ts::TR101_290Analyzer::ServiceContext::Assigned;}
bool is_pat(const ts::TR101_290Analyzer::ServiceContext& ctx) {return ctx._type == ts::TR101_290Analyzer::ServiceContext::Pat;}
bool is_pmt(const ts::TR101_290Analyzer::ServiceContext& ctx) {return ctx._type == ts::TR101_290Analyzer::ServiceContext::Pmt;}
bool is_table(const ts::TR101_290Analyzer::ServiceContext& ctx) {return ctx._type == ts::TR101_290Analyzer::ServiceContext::Pmt || ctx._type == ts::TR101_290Analyzer::ServiceContext::Pat;}

ErrorState get_sync_byte_err(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.sync_byte_error, true};}
ErrorState get_pat_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pat_error, is_pat(ctx)};}
ErrorState get_pat_error2(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pat_error_2,is_pat(ctx)};}
ErrorState get_cc_err(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.cc_error, true};}
ErrorState get_pmt_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pmt_error, is_pmt(ctx)};}
ErrorState get_pmt_error2(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pmt_error_2, is_pmt(ctx)};}
ErrorState get_pid_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pid_error, !is_pmt(ctx) && !is_pat(ctx)};}
ErrorState get_transport_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.transport_error, true};}
ErrorState get_crc_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.crc_error, is_table(ctx)};}
ErrorState get_pcr_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pcr_error, ctx.has_pcr};}
ErrorState get_repetition_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pcr_repetition_error, ctx.has_pcr};}
ErrorState get_discontinuity_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pcr_discontinuity_indicator_error, ctx.has_pcr};}
ErrorState get_accuracy_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pcr_accuracy_error, ctx.has_pcr};}
ErrorState get_pts_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.ptr_error, is_pes(ctx)};}
ErrorState get_cat_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.cat_error, false};} // todo

void ts::TR101_290Analyzer::report(std::ostream& stm, int& opt, Report& rep)
{
    stm << "Priority 1 Errors:\n";
    // todo: TS_sync_loss may not be a valid test in IP-based systems and covered by Sync_byte_error.
    // print_real("TS_sync_loss", NULL, stm, _services);

    print_real("Sync_byte_error", get_sync_byte_err, stm, _services);

    // todo: make sure that we don't have data on the PAT pid that is not a valid table.
    print_real("PAT_error", get_pat_error, stm, _services);
    print_real("PAT_error2", get_pat_error2, stm, _services);
    print_real("Continuity_count_error", get_cc_err, stm, _services);

    // todo: make sure that we don't have data on the PMT pid that is not a valid table.
    print_real("PMT_error", get_pmt_error, stm, _services);
    print_real("PMT_error_2", get_pmt_error2, stm, _services);

    print_real("PID_error", get_pid_error, stm, _services);

    stm << "\nPriority 2 Errors:\n";
    print_real("Transport_error", get_transport_error, stm, _services);

    // todo: CRC validation can be done per PID.
    SectionDemux::Status status;
    _demux.getStatus(status);
    print_custom("CRC_error", status.wrong_crc, stm, _services);


    print_real("PCR_error", get_pcr_error, stm, _services);
    print_real("PCR_repetition_error", get_repetition_error, stm, _services);
    print_real("PCR_discontinuity_indicator_error", get_discontinuity_error, stm, _services);
    print_real("PCR_accuracy_error", get_accuracy_error, stm, _services);
    print_real("PTS_error", get_pts_error, stm, _services);
    print_real("CAT_error", get_cat_error, stm, _services);
}

void ts::TR101_290Analyzer::reset()
{
    for (auto it = _services.begin(); it != _services.end(); ++it) {
        it->second->sync_byte_error = 0;
        it->second->pat_error = 0;
        it->second->pat_error_2 = 0;
        it->second->cc_error = 0;
        it->second->pmt_error = 0;
        it->second->pmt_error_2 = 0;
        it->second->pid_error = 0;
        it->second->transport_error = 0;
        it->second->crc_error = 0;
        it->second->pcr_error = 0;
        it->second->pcr_repetition_error = 0;
        it->second->pcr_discontinuity_indicator_error = 0;
        it->second->pcr_accuracy_error = 0;
        it->second->ptr_error = 0;
        it->second->cat_error = 0;
    }
}