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
#include <tsGrid.h>
#include <tsPAT.h>
#include <tsPMT.h>
#include <tsSectionDemux.h>

ts::TR101_290Analyzer::TR101_290Analyzer(DuckContext& duck): _duck(duck) {
    auto ptr = std::make_shared<ts::TR101_290Analyzer::ServiceContext>(0, ServiceContext::ServiceContextType::Pat);
    _services.insert(std::make_pair(0, ptr));

    _demux.addPID(PID_PAT);
    _demux.addPID(PID_CAT);
}

void ts::TR101_290Analyzer::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    auto service = getService(table.sourcePID());

    if (table.tableId() == TID_PAT && service->_type == ServiceContext::Pat) {
        const auto pat = PAT(_duck, table);

        // Assign PMTs.
        for (auto [service_id, pid] : pat.pmts) {
            auto s2 = getService(pid);
            s2->_type = ServiceContext::Pmt;
            s2->_pmt_service_id = service_id;
            _demux.addPID(pid);
        }

        // Remove PMT assignments.
        for (auto it = _services.begin(); it != _services.end(); ++it) {
            if (it->second->_type == ServiceContext::Pmt && !pat.pmts.contains(it->second->_pmt_service_id)) {
                it->second->_type = ServiceContext::Unassigned;
                _demux.removePID(it->first);
            }
        }
    } else if (table.tableId() == TID_PMT && service->_type == ServiceContext::Pmt) {
        const auto pmt = PMT(_duck, table);

        // Ensure all PIDs are assigned to this service.
        for (auto it = pmt.streams.begin(); it != pmt.streams.end(); it++) {
            const auto s2 = getService(it->first);
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
    }
}

void ts::TR101_290Analyzer::handleSection(SectionDemux& demux, const Section& table)
{
    if (table.sectionNumber() != 0) {
        // we only care about the first packet that has arrived.
        return;
    }

    auto service = getService(table.sourcePID());

    if (table.sourcePID() == PID_PAT && table.tableId() != TID_PAT) {
        service->pat_err.count++;
        service->pat_err2.count++;
    }

    if (service->_type == ServiceContext::Pmt && table.tableId() != TID_PMT) {
        service->pmt_err.count++;
        service->pmt_err2.count++;
    }

    if (table.tableId() == TID_PAT && service->_type == ServiceContext::Pat) {
        service->_type = ServiceContext::Pat;
        if (service->_last_table_ts != INVALID_PCR) {
            auto diff = long(_currentTimestamp - service->_last_table_ts);
            service->pat_err.pushSysClockFreq(diff);
            service->pat_err2.pushSysClockFreq(diff);
            auto limit = 500llu * SYSTEM_CLOCK_FREQ / 1000;
            if (diff > limit) {
                service->pat_err.count++;
                service->pat_err2.count++;
            }
        }
        service->_last_table_ts = _currentTimestamp;
    }
    else if (table.tableId() == TID_PMT && service->_type == ServiceContext::Pmt) {
        if (service->_last_table_ts != INVALID_PCR) {
            auto diff = long(_currentTimestamp - service->_last_table_ts);
            service->pmt_err.pushSysClockFreq(diff);
            service->pmt_err2.pushSysClockFreq(diff);
            auto limit = 500llu * SYSTEM_CLOCK_FREQ / 1000;
            if (diff > limit) {
                service->pmt_err.count++;
                service->pmt_err2.count++;
            }
        }
        service->_last_table_ts = _currentTimestamp;
    }
    else if (table.tableId() == TID_CAT) {
        // todo: figure out a timeout for this so this isn't true forever.
        _has_cat = true;
    }
}
void ts::TR101_290Analyzer::handleInvalidSection(SectionDemux& demux, const DemuxedData& data) {
    auto _service = getService(data.sourcePID());
    // todo: it is not guaranteed that it is a CRC error that causes this issue.
    _service->crc_error++;
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

void ts::TR101_290Analyzer::processPacket(ServiceContext& ctx, const TSPacket& pkt, const TSPacketMetadata& mdata) const
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
            ctx.pat_err.count++;
            ctx.pat_err2.count++;
        }
    } else if (ctx._type == ServiceContext::Pmt) {
        if (pkt.getScrambling()) {
            ctx.pmt_err.count++;
            ctx.pmt_err2.count++;
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
    if (ctx._type != ServiceContext::Unassigned) {
        const auto dur = long(pkt_ts - ctx.last_packet_ts);
        ctx.pid_err.pushSysClockFreq(dur);

        if (dur > 5 * SYSTEM_CLOCK_FREQ) {
            ctx.pid_err.count++;
        }
    }

    // PTS error.
    if (pkt.hasPTS()) {
        if (ctx.last_pts_ts != INVALID_PTS) {
            auto dur = long(pkt_ts - ctx.last_pts_ts);
            ctx.pts_err.pushSysClockFreq(dur);
            if (dur > 700 * SYSTEM_CLOCK_FREQ /  1000) {
                ctx.pts_err.count++;
            }
        }
        ctx.last_pts_ts = pkt_ts;
    }

    // PCR error.
    if (pkt.hasPCR()) {
        auto pcr = pkt.getPCR();
        if (ctx.last_pcr_val != INVALID_PCR && ctx.last_pcr_ts != INVALID_PCR) {
            auto pcr_dist = pcr - ctx.last_pcr_val;
            ctx.pcr_discontinuity_err.pushSysClockFreq((int)pcr_dist);

            if (pcr_dist > 100 * SYSTEM_CLOCK_FREQ /  1000) {
                ctx.pcr_discontinuity_err.count++;
                ctx.pcr_error++;
            }

            auto pkt_dist = pkt_ts - ctx.last_pcr_ts;
            ctx.pcr_repetition_err.pushSysClockFreq((int)pkt_dist);

            if (pkt_dist > 100 * SYSTEM_CLOCK_FREQ /  1000) {
                ctx.pcr_repetition_err.count++;
                ctx.pcr_error++;
            }

            auto delta = pcr_dist - (int64_t(_packetIndex - ctx.last_pcr_ctr) * PKT_SIZE_BITS * SYSTEM_CLOCK_FREQ / _bitrate);
            ctx.pcr_accuracy_err.pushSysClockFreq(delta.toInt());

            if (llabs(delta.toInt64()) > 500ll * int64_t(SYSTEM_CLOCK_FREQ) / int64_t(1e9)) {
                ctx.pcr_accuracy_err.count++;
            }
        }

        ctx.last_pcr_val = pcr;
        ctx.last_pcr_ts = pkt_ts;
        ctx.last_pcr_ctr = _packetIndex; //_tsp->pluginPackets();
    }

    ctx.last_packet_ts = pkt_ts;
    ctx.last_cc = pkt.getCC();
}

void ts::TR101_290Analyzer::feedPacket(const TSPacket& packet, const TSPacketMetadata& mdata, BitRate bitrate, PacketCounter packetIndex)
{
    _currentTimestamp = mdata.getInputTimeStamp().count();
    auto service = getService(packet.getPID());
    _bitrate = bitrate;
    _packetIndex = packetIndex;
    processPacket(*service, packet, mdata);
    _demux.feedPacket(packet);
}

const char* ERR = "[ERR]";
const char* OK  = "[OK] ";
const char* NA  = "[N/A]";
struct ErrorState {
    long count;
    bool show;
    std::string str{};
};

typedef std::function<ErrorState(const ts::TR101_290Analyzer::ServiceContext&)> get_func;


static long count(const get_func& fn, std::map<ts::PID,  std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
{
    long count = 0;
    for (auto it = _services.begin(); it != _services.end(); it++) {
        auto val = fn(*it->second);
        if (val.show)
            count += val.count;
    }
    return count;
}

static void print(const char* name, const get_func& fn,  std::ostream& stm, std::map<ts::PID, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
{
    for (auto it = _services.begin(); it != _services.end(); it++) {
        auto val = fn(*it->second);
        if (val.show)
            stm << "\t" << (val.count == 0 ? OK : ERR) << "PID 0x" << std::format("{:X}", it->second->_pid) << ": " << val.count << " " << val.str << "\n";
    }
}

static void print_real(const char* name, const get_func& fn, std::ostream& stm, std::map<ts::PID, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
{
    auto count = ::count(fn , _services);

    stm << (count == 0 ? OK : ERR) << " " << name << ": " << count << "\n";
    print(name, fn, stm, _services);
}

static void print_custom(const char* name, int count, std::ostream& stm, std::map<ts::PID, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
{
    stm << (count == 0 ? OK : ERR) << " " << name << ": " << count << "\n";
}

bool is_pes(const ts::TR101_290Analyzer::ServiceContext& ctx) {return ctx._type == ts::TR101_290Analyzer::ServiceContext::Assigned;}
bool is_pat(const ts::TR101_290Analyzer::ServiceContext& ctx) {return ctx._type == ts::TR101_290Analyzer::ServiceContext::Pat;}
bool is_pmt(const ts::TR101_290Analyzer::ServiceContext& ctx) {return ctx._type == ts::TR101_290Analyzer::ServiceContext::Pmt;}
bool is_table(const ts::TR101_290Analyzer::ServiceContext& ctx) {return ctx._type == ts::TR101_290Analyzer::ServiceContext::Pmt || ctx._type == ts::TR101_290Analyzer::ServiceContext::Pat;}

ErrorState get_sync_byte_err(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.sync_byte_error, true};}
ErrorState get_pat_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pat_err.count, is_pat(ctx), ctx.pat_err.to_string()};}
ErrorState get_pat_error2(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pat_err2.count,is_pat(ctx), ctx.pat_err2.to_string()};}
ErrorState get_cc_err(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.cc_error, true};}
ErrorState get_pmt_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pmt_err.count, is_pmt(ctx), ctx.pmt_err.to_string()};}
ErrorState get_pmt_error2(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pmt_err2.count, is_pmt(ctx), ctx.pmt_err2.to_string()};}
ErrorState get_pid_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pid_err.count, !is_pmt(ctx) && !is_pat(ctx), ctx.pid_err.to_string()};}
ErrorState get_transport_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.transport_error, true};}
ErrorState get_crc_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.crc_error, is_table(ctx)};}
ErrorState get_pcr_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pcr_error, ctx.has_pcr};}
ErrorState get_repetition_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pcr_repetition_err.count, ctx.has_pcr, ctx.pcr_repetition_err.to_string()};}
ErrorState get_discontinuity_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pcr_discontinuity_err.count, ctx.has_pcr, ctx.pcr_discontinuity_err.to_string()};}
ErrorState get_accuracy_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pcr_accuracy_err.count, ctx.has_pcr, ctx.pcr_accuracy_err.to_string()};}
ErrorState get_pts_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pts_err.count, is_pes(ctx), ctx.pts_err.to_string()};}
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
    print_custom("CRC_error", long(status.wrong_crc), stm, _services);


    print_real("PCR_error", get_pcr_error, stm, _services);
    print_real("PCR_repetition_error", get_repetition_error, stm, _services);
    print_real("PCR_discontinuity_indicator_error", get_discontinuity_error, stm, _services);
    print_real("PCR_accuracy_error", get_accuracy_error, stm, _services);
    print_real("PTS_error", get_pts_error, stm, _services);
    print_real("CAT_error", get_cat_error, stm, _services);
}

void ts::TR101_290Analyzer::reset()
{
    for (auto & [_, service] : _services) {
        service->sync_byte_error = 0;
        service->pat_err.clear();
        service->pat_err2.clear();
        service->cc_error = 0;
        service->pmt_err.clear();
        service->pmt_err2.clear();
        service->pid_err.clear();
        service->transport_error = 0;
        service->crc_error = 0;
        service->pcr_error = 0;
        service->pcr_repetition_err.clear();
        service->pcr_discontinuity_err.clear();
        service->pcr_accuracy_err.clear();
        service->pts_err.clear();
        service->cat_error = 0;
    }
}