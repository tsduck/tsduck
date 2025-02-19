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
#include <tsArgs.h>
#include <tsBinaryTable.h>
#include <tsPAT.h>
#include <tsPMT.h>
#include <tsjsonObject.h>

// Defined in TR 101 290 Section 5.2.1
#define PAT_INTERVAL (500l * SYSTEM_CLOCK_FREQ / 1000)

// Defined in TR 101 290 Section 5.2.1
#define PMT_INTERVAL (500l * SYSTEM_CLOCK_FREQ / 1000)

// To the best of my knowledge, there is no specification that defines how frequently the CAT should be transmitted.
#define CAT_VALID_INTERVAL (10l * SYSTEM_CLOCK_FREQ)

// Defined in TR 101 290 Section 5.2.2
#define PTS_REPETITION_INTERVAL (700l * SYSTEM_CLOCK_FREQ / 1000)

#define PCR_DISCONTINUITY_LIMIT (100l * SYSTEM_CLOCK_FREQ /  1000)
#define PCR_REPETITION_LIMIT (100l * SYSTEM_CLOCK_FREQ /  1000)
#define PCR_ACCURACY_LIMIT_DOUBLE (500.0 / 1e9)

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

void ts::TR101_290Analyzer::handleSection(SectionDemux& demux, const Section& section)
{
    if (section.sectionNumber() != 0) {
        // we only care about the first packet that has arrived.
        return;
    }

    auto service = getService(section.sourcePID());

    if (section.sourcePID() == PID_PAT && section.tableId() != TID_PAT) {
        service->pat_err.count++;
        service->pat_err2.count++;
    }

    if (service->_type == ServiceContext::Pmt && section.tableId() != TID_PMT) {
        service->pmt_err.count++;
        service->pmt_err2.count++;
    }

    if (section.tableId() == TID_PAT && service->_type == ServiceContext::Pat) {
        service->_type = ServiceContext::Pat;
        if (service->_last_table_ts != INVALID_PCR) {
            auto diff = long(_currentTimestamp - service->_last_table_ts);
            service->pat_err.pushSysClockFreq(diff);
            service->pat_err2.pushSysClockFreq(diff);
            if (diff > PAT_INTERVAL) {
                service->pat_err.count++;
                service->pat_err2.count++;
            }
        }
        service->_last_table_ts = _currentTimestamp;
    }
    else if (section.tableId() == TID_PMT && service->_type == ServiceContext::Pmt) {
        if (service->_last_table_ts != INVALID_PCR) {
            auto diff = long(_currentTimestamp - service->_last_table_ts);
            service->pmt_err.pushSysClockFreq(diff);
            service->pmt_err2.pushSysClockFreq(diff);
            if (diff > PMT_INTERVAL) {
                service->pmt_err.count++;
                service->pmt_err2.count++;
            }
        }
        service->_last_table_ts = _currentTimestamp;
    }
    else if (section.tableId() == TID_CAT) {
        _lastCatIndex = _packetIndex;

        for (auto & [_, s2] : _services) {
            s2->cat_error = 0;
        }
    }
}
void ts::TR101_290Analyzer::handleInvalidSection(SectionDemux& demux, const DemuxedData& data, Section::Status status) {
    auto _service = getService(data.sourcePID());
    if (status == Section::Status::INV_CRC32)
        _service->crc_error++;
}

std::shared_ptr<ts::TR101_290Analyzer::ServiceContext> ts::TR101_290Analyzer::getService(PID pid)
{
    auto it = _services.find(pid);
    if (it == _services.end()) {
        auto service = std::make_shared<ServiceContext>(ServiceContext {
            ._pid = pid,
            ._type = ServiceContext::Unassigned});
        _services.insert(std::make_pair(pid, service));
        return service;
    }
    return it->second;
}

void ts::TR101_Options::defineArgs(Args& args)
{
    json.defineArgs(args, true, u"JSON");

    args.option(u"show-report", 0, Args::NONE);
    args.help(u"show-report", u"Show an TR 101-290 analyzer report before exiting. By default this is enabled, unless JSON is set.");
}

bool ts::TR101_Options::loadArgs(DuckContext& duck, Args& args)
{
    if (!json.loadArgs(duck, args)) return false;

    show_report = !json.useJSON() || args.present(u"show-report");
    return true;
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

    if (pkt.isScrambled()) {
        bool cat_invalid = _lastCatIndex == INVALID_PACKET_COUNTER || (_packetIndex - _lastCatIndex > CAT_VALID_INTERVAL);

        if (cat_invalid) {
            ctx.cat_error++;
        }
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
            // TODO: The limitation to 700 ms should not be applied to still pictures.
            // From TR 101 290 Table 5.0b
            if (dur > PTS_REPETITION_INTERVAL) {
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

            if (pcr_dist > PCR_DISCONTINUITY_LIMIT) {
                ctx.pcr_discontinuity_err.count++;
                ctx.pcr_error++;
            }

            auto pkt_dist = pkt_ts - ctx.last_pcr_ts;
            ctx.pcr_repetition_err.pushSysClockFreq((int)pkt_dist);

            if (pkt_dist > PCR_REPETITION_LIMIT) {
                ctx.pcr_repetition_err.count++;
                ctx.pcr_error++;
            }

            auto pcr_accuracy_sec = pcr_dist - (int64_t(_packetIndex - ctx.last_pcr_ctr) * PKT_SIZE_BITS * SYSTEM_CLOCK_FREQ / _bitrate);
            ctx.pcr_accuracy_err.pushSysClockFreq(pcr_accuracy_sec.toInt());

            if (fabs(pcr_accuracy_sec.toDouble()) > PCR_ACCURACY_LIMIT_DOUBLE) {
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

const char16_t* ERR = u"[ERR] ";
const char16_t* OK  = u"[OK]  ";
const char16_t* NA  = u"[N/A] ";
struct ErrorState {
    long count=0;
    bool show=false;
    std::optional<ts::TR101_290Analyzer::ServiceContext::IntMinMax> min_max{};
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

static void print(const char16_t* name, const get_func& fn,  std::ostream& stm, std::map<ts::PID, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
{
    for (auto it = _services.begin(); it != _services.end(); it++) {
        auto val = fn(*it->second);
        if (val.show) {
            ts::UString min_max;
            if (val.min_max)
                min_max  = val.min_max->to_string();

            ts::UString fmt;
            fmt.format(u"{:X}", it->second->_pid);
            stm << u"\t" << (val.count == 0 ? OK : ERR) << u"PID 0x" << fmt << u" ("<<it->second->_pid<< u"): " << val.count << u" " << min_max << u"\n";
        }
    }
}

static void print_real(const char16_t* name, const get_func& fn, std::ostream& stm, std::map<ts::PID, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
{
    auto count = ::count(fn , _services);

    stm << (count == 0 ? OK : ERR) << u" " << name << u": " << count << u"\n";
    print(name, fn, stm, _services);
}

// static void print_custom(const char* name, long count, std::ostream& stm, std::map<ts::PID, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
// {
//     stm << (count == 0 ? OK : ERR) << " " << name << ": " << count << "\n";
// }



static void json(const char16_t* name, const get_func& fn,  ts::json::Value& stm, ts::json::Value& pids, std::map<ts::PID, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
{
    for (auto it = _services.begin(); it != _services.end(); it++) {
        auto val = fn(*it->second);
        if (val.show) {
            stm.add(u"count", val.count);

            ts::json::Value& v(pids.query(ts::UString::Decimal(it->first, 0, true, u""), true));
            ts::json::Value& v2(v.query(name, true));
            v2.add(u"curr", val.count);
            if (val.min_max) {
                val.min_max->defineJson(v2);
            }
        }
    }
}

static void json_real(const char* name, const get_func& fn, ts::json::Value& stm, std::map<ts::PID, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
{
    auto count = ::count(fn , _services);

    stm.add(ts::UString::FromUTF8(name), count);
    ts::json::Value& pids(stm.query(u"pids", true));
    json(ts::UString::FromUTF8(name).data(), fn, stm, pids, _services);
}

bool is_pes(const ts::TR101_290Analyzer::ServiceContext& ctx) {return ctx._type == ts::TR101_290Analyzer::ServiceContext::Assigned;}
bool is_pat(const ts::TR101_290Analyzer::ServiceContext& ctx) {return ctx._type == ts::TR101_290Analyzer::ServiceContext::Pat;}
bool is_pmt(const ts::TR101_290Analyzer::ServiceContext& ctx) {return ctx._type == ts::TR101_290Analyzer::ServiceContext::Pmt;}
bool is_table(const ts::TR101_290Analyzer::ServiceContext& ctx) {return ctx._type == ts::TR101_290Analyzer::ServiceContext::Pmt || ctx._type == ts::TR101_290Analyzer::ServiceContext::Pat;}

ErrorState get_sync_byte_err(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.sync_byte_error, true};}
ErrorState get_pat_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pat_err.count, is_pat(ctx), ctx.pat_err};}
ErrorState get_pat_error2(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pat_err2.count,is_pat(ctx), ctx.pat_err2};}
ErrorState get_cc_err(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.cc_error, true};}
ErrorState get_pmt_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pmt_err.count, is_pmt(ctx), ctx.pmt_err};}
ErrorState get_pmt_error2(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pmt_err2.count, is_pmt(ctx), ctx.pmt_err2};}
ErrorState get_pid_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pid_err.count, !is_pmt(ctx) && !is_pat(ctx), ctx.pid_err};}
ErrorState get_transport_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.transport_error, true};}
ErrorState get_crc_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.crc_error, is_table(ctx)};}
ErrorState get_pcr_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pcr_error, ctx.has_pcr};}
ErrorState get_repetition_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pcr_repetition_err.count, ctx.has_pcr, ctx.pcr_repetition_err};}
ErrorState get_discontinuity_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pcr_discontinuity_err.count, ctx.has_pcr, ctx.pcr_discontinuity_err};}
ErrorState get_accuracy_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pcr_accuracy_err.count, ctx.has_pcr, ctx.pcr_accuracy_err};}
ErrorState get_pts_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.pts_err.count, is_pes(ctx), ctx.pts_err};}
ErrorState get_cat_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.cat_error, true};}

void ts::TR101_290Analyzer::report(std::ostream& stm, int& opt, Report& rep)
{
    stm << "Priority 1 Errors:\n";
    // todo: TS_sync_loss may not be a valid test in IP-based systems and covered by Sync_byte_error.
    // print_real("TS_sync_loss", NULL, stm, _services);
    // print_real("Sync_byte_error", get_sync_byte_err, stm, _services);

    // todo: make sure that we don't have data on the PAT pid that is not a valid table.
    print_real(u"PAT_error", get_pat_error, stm, _services);
    print_real(u"PAT_error2", get_pat_error2, stm, _services);
    print_real(u"Continuity_count_error", get_cc_err, stm, _services);

    // todo: make sure that we don't have data on the PMT pid that is not a valid table.
    print_real(u"PMT_error", get_pmt_error, stm, _services);
    print_real(u"PMT_error_2", get_pmt_error2, stm, _services);

    print_real(u"PID_error", get_pid_error, stm, _services);

    stm << "\nPriority 2 Errors:\n";
    print_real(u"Transport_error", get_transport_error, stm, _services);
    print_real(u"CRC_error", get_crc_error, stm, _services);
    print_real(u"PCR_error", get_pcr_error, stm, _services);
    print_real(u"PCR_repetition_error", get_repetition_error, stm, _services);
    print_real(u"PCR_discontinuity_indicator_error", get_discontinuity_error, stm, _services);
    print_real(u"PCR_accuracy_error", get_accuracy_error, stm, _services);
    print_real(u"PTS_error", get_pts_error, stm, _services);
    print_real(u"CAT_error", get_cat_error, stm, _services);
}

void ts::TR101_290Analyzer::reportJSON(TR101_Options& opt, std::ostream& stm, const UString& title, Report& rep)
{
    // JSON root.
    json::Object root;

    // Add user-supplied title.
    if (!title.empty()) {
        root.add(u"title", title);
    }

    json::Value& obj(root.query(u"tr101", true));
    // if (_ts_id.has_value()) {
    //     ts.add(u"id", *_ts_id);
    // }

    // todo: TS_sync_loss may not be a valid test in IP-based systems and covered by Sync_byte_error.
    // print_real("TS_sync_loss", NULL, stm, _services);
    // print_real("Sync_byte_error", get_sync_byte_err, stm, _services);

    // todo: make sure that we don't have data on the PAT pid that is not a valid table.
    json_real("PAT_error", get_pat_error, obj, _services);
    json_real("PAT_error2", get_pat_error2, obj, _services);
    json_real("Continuity_count_error", get_cc_err, obj, _services);

    // todo: make sure that we don't have data on the PMT pid that is not a valid table.
    json_real("PMT_error", get_pmt_error, obj, _services);
    json_real("PMT_error_2", get_pmt_error2, obj, _services);

    json_real("PID_error", get_pid_error, obj, _services);

    json_real("Transport_error", get_transport_error, obj, _services);
    json_real("CRC_error", get_crc_error, obj, _services);
    json_real("PCR_error", get_pcr_error, obj, _services);
    json_real("PCR_repetition_error", get_repetition_error, obj, _services);
    json_real("PCR_discontinuity_indicator_error", get_discontinuity_error, obj, _services);
    json_real("PCR_accuracy_error", get_accuracy_error, obj, _services);
    json_real("PTS_error", get_pts_error, obj, _services);
    json_real("CAT_error", get_cat_error, obj, _services);

    opt.json.report(root, stm, rep);
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