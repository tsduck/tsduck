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
#include <utility>
#include <tsArgs.h>
#include <tsBinaryTable.h>
#include <tsPAT.h>
#include <tsPMT.h>
#include <tsjsonObject.h>

// Defined in TR 101 290 Section 5.2.1
#define PAT_INTERVAL (500l * ts::SYSTEM_CLOCK_FREQ / 1000)

// Defined in TR 101 290 Section 5.2.1
#define PMT_INTERVAL (500l * ts::SYSTEM_CLOCK_FREQ / 1000)

// To the best of my knowledge, there is no specification that defines how frequently the CAT should be transmitted.
#define CAT_VALID_INTERVAL (10l * ts::SYSTEM_CLOCK_FREQ)

// Defined in TR 101 290 Section 5.2.2
#define PTS_REPETITION_INTERVAL (700l * ts::SYSTEM_CLOCK_FREQ / 1000)

#define PCR_DISCONTINUITY_LIMIT (100l * ts::SYSTEM_CLOCK_FREQ /  1000)
#define PCR_REPETITION_LIMIT (100l * ts::SYSTEM_CLOCK_FREQ /  1000)
#define PCR_ACCURACY_LIMIT_DOUBLE (500.0 / 1e9)

ts::UString ts::TR101_290Analyzer::IntMinMax::to_string() const
{
    if (is_ms) {
        UString maxStr;
        if (this->max != LONG_MIN) {
            UString fmt;
            fmt.format(u"%.2f", double(this->max) / 1000000.0);
            maxStr = u" MAX: " + fmt + u"ms";
        }
        UString minStr;
        if (this->min != LONG_MAX) {
            UString fmt;
            fmt.format(u"%.2f", double(this->min) / 1000000.0);
            minStr = u" MIN: " + fmt + u"ms";
        }

        UString fmt;
        fmt.format(u"%.2f", double(curr) / 1000000.0);
        return minStr + maxStr + u" CURR: " + fmt + u"ms";
    }
    else {
        UString maxStr;
        if (this->max != LONG_MIN) {
            maxStr = u" MAX: " + UString::Decimal(this->max) + u"ns";
        }
        UString minStr;
        if (this->min != LONG_MAX) {
            minStr = u" MIN: " + UString::Decimal(this->min) + u"ns";
        }
        return minStr + maxStr + u" CURR: " + UString::Decimal(curr) + u"ns";
    }
}

void ts::TR101_290Analyzer::IntMinMax::defineJson(json::Value& value) const
{
    if (this->max != LONG_MIN) {
        value.add(u"max", double(this->max) / 1e9);
    }
    if (this->min != LONG_MAX) {
        value.add(u"min", double(this->min) / 1e9);
    }
    value.add(u"curr", double(this->curr) / 1e9);
}

void ts::TR101_290Analyzer::IntMinMax::pushNs(long val)
{
    curr = val;
    if (val < min)
        min = val;
    if (val > max)
        max = val;
}

void ts::TR101_290Analyzer::IntMinMax::pushSysClockFreq(long val)
{
    const int ns = int(val * 1e9l / ts::SYSTEM_CLOCK_FREQ);
    pushNs(ns);
}

void ts::TR101_290Analyzer::IntMinMax::clear()
{
    curr = 0;
    min = LONG_MAX;
    max = LONG_MIN;
    count = 0;
}

ts::TR101_290Analyzer::Indicator::Indicator(UString _name, bool _show_value, uint64_t _value_timeout) :
    name(std::move(_name)),
    show_value(_show_value),
    value_timeout(_value_timeout)
{

}

void ts::TR101_290Analyzer::Indicator::timeout(bool timeout)
{
    if (timeout && !in_timeout)
        in_err_count++;

    in_timeout = timeout;
}

void ts::TR101_290Analyzer::Indicator::timeoutAfter(uint64_t now, uint64_t max_val)
{
    if (prev_ts != ts::INVALID_PCR && now - prev_ts > max_val) {
        timeout(true);
    }
}

void ts::TR101_290Analyzer::Indicator::update(uint64_t now, bool in_error)
{
    prev_ts = now;

    if (in_error)
        in_err_count++;

    if (in_timeout)
        in_timeout = false;
}

void ts::TR101_290Analyzer::Indicator::update(uint64_t now, bool in_error, int64_t value)
{
    minMax.pushSysClockFreq(value);
    update(now, in_error);
}

void ts::TR101_290Analyzer::Indicator::clear()
{
    in_err_count = 0;
    in_timeout = false;
}

ts::TR101_290Analyzer::ServiceContext::ServiceContext(PID _pid, ServiceContextType _type) :
    pid(_pid), type(_type),
    PAT_error(u"PAT_error", true),
    PAT_error_2(u"PAT_error_2", true),
    CC_error(u"Continuity_count_error", false),
    PMT_error(u"PMT_error", true),
    PMT_error_2(u"PMT_error_2", true),
    PID_error(u"PID_error", true),
    Transport_error(u"Transport_error", false),
    CRC_error(u"CRC_error", false),
    PCR_error(u"PCR_error", true),
    PCR_repetition_error(u"PCR_repetition_error", true),
    PCR_discontinuity_indicator_error(u"PCR_discontinuity_indicator_error", true),
    PCR_accuracy_error(u"PCR_accuracy_error", true),
    PTS_error(u"PTS_error", true),
    CAT_error(u"CAT_error", false)
{
}

void ts::TR101_290Analyzer::ServiceContext::clear() {
    PAT_error.clear();
    PAT_error_2.clear();
    CC_error.clear();
    PMT_error.clear();
    PMT_error_2.clear();
    PID_error.clear();
    Transport_error.clear();
    CRC_error.clear();
    PCR_error.clear();
    PCR_repetition_error.clear();
    PCR_discontinuity_indicator_error.clear();
    PCR_accuracy_error.clear();
    PTS_error.clear();
    CAT_error.clear();
}

ts::TR101_290Analyzer::TR101_290Analyzer(DuckContext& duck) :
    _duck(duck)
{
    auto ptr = std::make_shared<ServiceContext>(0, ServiceContext::ServiceContextType::Pat);

    _services.insert(std::make_pair(0, ptr));
    _demux.addPID(PID_CAT);
    _demux.addPID(PID_PAT);
    _demux.addPID(PID_NIT);
    _demux.addPID(PID_EIT);
    _demux.addPID(PID_SDT);
    _demux.addPID(PID_TOT);
}

void ts::TR101_290Analyzer::handleTable(SectionDemux& demux, const BinaryTable& table)
{

    if (auto service = getService(table.sourcePID()); table.tableId() == TID_PAT && service->type == ServiceContext::Pat) {
        const auto pat = PAT(_duck, table);

        // Assign PMTs.
        for (auto [service_id, pid] : pat.pmts) {
            auto s2 = getService(pid);
            s2->type = ServiceContext::Pmt;
            s2->pmt_service_id = service_id;
            _demux.addPID(pid);
        }

        // Remove PMT assignments.
        for (auto & _service : _services) {
            if (_service.second->type == ServiceContext::Pmt && !pat.pmts.contains(_service.second->pmt_service_id)) {
                _service.second->type = ServiceContext::Unassigned;
                _demux.removePID(_service.first);
            }
        }
    } else if (table.tableId() == TID_PMT && service->type == ServiceContext::Pmt) {
        const auto pmt = PMT(_duck, table);

        // Ensure all PIDs are assigned to this service.
        for (const auto & [pid, _] : pmt.streams) {
            const auto s2 = getService(pid);
            s2->type = ServiceContext::Assigned;
            s2->pmt_service_id = service->pmt_service_id;
        }

        // Remove PIDs that are Assigned to this service ID.
        for (auto & [pid, service2] : _services) {
            if (service2->type == ServiceContext::Assigned && service2->pmt_service_id == service2->pmt_service_id) {
                if (!pmt.streams.contains(pid)) {
                    service2->type = ServiceContext::Unassigned;
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

    if (section.sourcePID() == PID_PAT) {
        // a PID 0x0000 does not contain a table_id 0x00 (i.e.a PAT)
        service->PAT_error.update(_currentTimestamp, section.tableId() != TID_PAT);
        service->PAT_error_2.update(_currentTimestamp, section.tableId() != TID_PAT);

        if (section.tableId() == TID_PAT) {
            if (service->last_table_ts != INVALID_PCR) {
                // PID 0x0000 does not occur at least every 0,5 s
                auto diff = long(_currentTimestamp - service->last_table_ts);
                service->PAT_error_2.update(_currentTimestamp, false, diff);
                service->PAT_error_2.update(_currentTimestamp, false, diff);
            }

            service->last_table_ts = _currentTimestamp;
        }
    } else if (service->type == ServiceContext::Pmt && section.tableId() == TID_PMT) {
        if (service->last_table_ts != INVALID_PCR) {
            // PID 0x0000 does not occur at least every 0,5 s
            auto diff = long(_currentTimestamp - service->last_table_ts);
            service->PMT_error.update(_currentTimestamp, false, diff);
            service->PMT_error_2.update(_currentTimestamp, false, diff);
        }

        service->last_table_ts = _currentTimestamp;
    } else if (section.sourcePID() == PID_CAT) {
        if (section.tableId() == TID_CAT) {
            _lastCatIndex = _currentTimestamp;
        } else {
            // Section with table_id other than 0x01 (i.e. not a CAT) found on PID 0x0001
            service->CAT_error.update(_currentTimestamp, true);
        }
    }
}

void ts::TR101_290Analyzer::handleInvalidSection(SectionDemux& demux, const DemuxedData& data, Section::Status status)
{
    auto service = getService(data.sourcePID());
    service->CRC_error.update(_currentTimestamp, status == Section::Status::INV_CRC32);

    if (data.sourcePID() == PID_PAT) {
        // a PID 0x0000 does not contain a table_id 0x00 (i.e.a PAT)
        service->PAT_error.update(_currentTimestamp, true);
        service->PAT_error_2.update(_currentTimestamp, true);
    }
}

void ts::TR101_290Analyzer::processPacket(ServiceContext& ctx, const TSPacket& pkt, const TSPacketMetadata& mdata) const
{
    // Priority 1 Errors
    // todo: TS_sync_loss
    // todo: Sync_byte_error

    // Scrambling_control_field is not 00 for PID 0x0000
    if (pkt.getPID() == PID_PAT && pkt.getScrambling()) {
        ctx.PAT_error.update(_currentTimestamp, true);
        ctx.PAT_error_2.update(_currentTimestamp, true);
    }

    // todo: Scrambling_control_field is not 00 for all PIDs containing sections with table_id 0x02

    // Process CC errors.
    bool repeat = false;
    // The continuity counter may be discontinuous when the discontinuity_indicator is set to '1'
    // In the case of a null packet the value of the continuity_counter is undefined.
    if (!pkt.getDiscontinuityIndicator() && pkt.getPID() != PID_NULL && ctx.last_cc != -1) {
        auto expected_cc = ctx.last_cc;

        // The continuity_counter shall not be incremented when the adaptation_field_control of the packet equals '00' or '10'.
        if (pkt.hasPayload()) {
            // In Transport Streams, duplicate packets may be sent as two, and only two, consecutive Transport Stream packets of the
            // same PID.
            // The duplicate packets shall have the same continuity_counter value as the original packet and the
            // adaptation_field_control field shall be equal to '01' or '11'.
            if (pkt.getCC() == expected_cc && !ctx.last_repeat) {
                repeat = true;
            } else {
                expected_cc = (expected_cc + 1) % CC_MAX;
            }
        }

        if (expected_cc != pkt.getCC()) {
            ctx.CC_error.update(_currentTimestamp, true);
        }
    }
    ctx.last_cc = pkt.getCC();
    ctx.last_repeat = repeat;

    ctx.PID_error.update(_currentTimestamp, false);

    // Priority 2 Errors
    ctx.Transport_error.update(_currentTimestamp, pkt.getTEI() == true);
    // CRC_error in handleInvalidSection

    if (pkt.hasPCR()) {
        auto pcr_val = pkt.getPCR();

        {
            // PCR discontinuity of more than 100 ms occurring
            // without specific indication.
            // Time interval between two consecutive PCR
            // values more than 100 ms
            if (ctx.last_pcr_ts != INVALID_PCR && !ctx.has_discontinuity) {
                auto error = int64_t(_currentTimestamp - ctx.last_pcr_ts);
                ctx.PCR_error.update(_currentTimestamp, error > PCR_DISCONTINUITY_LIMIT, error);
            }
        }

        // Time interval between two consecutive PCR
        // values more than 100 ms
        ctx.PCR_repetition_error.update(_currentTimestamp, false);

        {
            // The difference between two consecutive PCR
            // values(PCRi + 1 – PCRi) is outside the range of 0...100 ms
            // without the discontinuity_indicator set
            if (ctx.last_pcr_val != INVALID_PCR && !ctx.has_discontinuity) {
                auto error = int64_t(pcr_val) - int64_t(ctx.last_pcr_val);
                ctx.PCR_discontinuity_indicator_error.update(_currentTimestamp, error > PCR_DISCONTINUITY_LIMIT || error < 0, error);
            }
        }

        // PCR accuracy of selected programme is not within ± 500 ns
        {
            auto pcr_actual_dist = pcr_val - ctx.last_pcr_val;
            auto pcr_expected_dist = int64_t(_currentTimestamp - ctx.last_pcr_ts) * ts::PKT_SIZE_BITS * ts::SYSTEM_CLOCK_FREQ / _bitrate;
            auto pcr_accuracy = pcr_actual_dist - pcr_expected_dist;

            ctx.PCR_accuracy_error.update(_currentTimestamp, llabs(pcr_accuracy.toInt64()) < PCR_ACCURACY_LIMIT_DOUBLE, pcr_accuracy.toInt64());
        }

        ctx.last_pcr_ts = _currentTimestamp;
        ctx.last_pcr_val = pcr_val;
        ctx.has_discontinuity = false;
    }

    if (pkt.hasPTS()) {
        // PTS repetition period more than 700 ms
        if (ctx.last_pts_ts != INVALID_PCR) {
            auto error = int64_t(_currentTimestamp - ctx.last_pts_ts);
            // todo: The limitation to 700 ms should not be applied to still pictures.
            ctx.PTS_error.update(_currentTimestamp, error > PTS_REPETITION_INTERVAL, error);
        }
        ctx.last_pts_ts = _currentTimestamp;
    }

    // Packets with transport_scrambling_control not 00
    // present, but no section with table_id = 0x01(i.e. a CAT) present
    ctx.CAT_error.update(_currentTimestamp, pkt.getScrambling() && _currentTimestamp - _lastCatIndex > CAT_VALID_INTERVAL);
}

void ts::TR101_290Analyzer::processTimeouts(ServiceContext& ctx)
{
    // PID 0x0000 does not occur at least every 0,5 s
    ctx.PAT_error.timeoutAfter(_currentTimestamp, PAT_INTERVAL);

    // Sections with table_id 0x00 do not occur at least
    // every 0, 5 s on PID 0x0000.
    ctx.PAT_error_2.timeoutAfter(_currentTimestamp, PAT_INTERVAL);

    // Sections with table_id 0x02, (i.e. a PMT), do not
    // occur at least every 0, 5 s on the PID which is
    // referred to in the PAT
    ctx.PMT_error.timeoutAfter(_currentTimestamp, PMT_INTERVAL);
    ctx.PMT_error_2.timeoutAfter(_currentTimestamp, PMT_INTERVAL);

    ctx.PID_error.timeoutAfter(_currentTimestamp, PMT_INTERVAL);

    // PCR discontinuity of more than 100 ms occurring
    // without specific indication.
    if (!ctx.has_discontinuity) {
        ctx.PCR_error.timeoutAfter(_currentTimestamp, PCR_REPETITION_LIMIT);
    }

    // Time interval between two consecutive PCR
    // values more than 100 ms
    ctx.PCR_repetition_error.timeoutAfter(_currentTimestamp, PCR_REPETITION_LIMIT);

    // PTS repetition period more than 700 ms
    // todo: NOTE 3: The limitation to 700 ms should not be applied to still pictures.
    ctx.PTS_error.timeoutAfter(_currentTimestamp, PTS_REPETITION_INTERVAL);
}

std::shared_ptr<ts::TR101_290Analyzer::ServiceContext> ts::TR101_290Analyzer::getService(PID pid)
{
    auto it = _services.find(pid);
    if (it == _services.end()) {
        auto service = std::make_shared<ServiceContext>(pid, ServiceContext::ServiceContextType::Unassigned);
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
    if (!json.loadArgs(duck, args))
        return false;

    show_report = !json.useJSON() || args.present(u"show-report");
    return true;
}

void ts::TR101_290Analyzer::feedPacket(const TSPacket& packet, const TSPacketMetadata& mdata, const BitRate& bitrate, PacketCounter packetIndex)
{
    _currentTimestamp = mdata.getInputTimeStamp().count();
    auto service = getService(packet.getPID());
    _bitrate = bitrate;
    _packetIndex = packetIndex;
    processTimeouts(*service);
    processPacket(*service, packet, mdata);
    _demux.feedPacket(packet);
}

const char16_t* ERR = u"[ERR] ";
const char16_t* OK  = u"[OK]  ";
const char16_t* NA  = u"[N/A] ";
struct ErrorState {
    const ts::TR101_290Analyzer::Indicator& indicator;
    bool show = false;
};

typedef std::function<ErrorState(const ts::TR101_290Analyzer::ServiceContext&)> get_func;

static long count(const get_func& fn, const std::map<ts::PID, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
{
    long count = 0;
    for (auto & [pid, service] : _services) {
        auto val = fn(*service);
        if (val.show)
            count += val.indicator.in_err_count;
    }
    return count;
}

static void print(const char16_t* name, const get_func& fn,  std::ostream& stm, const std::map<ts::PID, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& services)
{
    for (auto & [pid, service] : services) {
        auto val = fn(*service);
        if (val.show) {
            ts::UString min_max;
            if (val.indicator.show_value)
                min_max  = val.indicator.minMax.to_string();

            ts::UString fmt;
            fmt.format(u"%X", service->pid);
            stm << u"\t" << (val.indicator.in_err_count == 0 ? OK : ERR) << u"PID 0x" << fmt << u" ("<<service->pid<< u"): " << val.indicator.in_err_count << u" " << min_max << u"\n";
        }
    }
}

static void print_real(const char16_t* name, const get_func& fn, std::ostream& stm, const std::map<ts::PID, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
{
    auto count = ::count(fn , _services);

    stm << (count == 0 ? OK : ERR) << u" " << name << u": " << count << u"\n";
    print(name, fn, stm, _services);
}

static void json(const char16_t* name, const get_func& fn,  ts::json::Value& stm, ts::json::Value& pids, const std::map<ts::PID, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
{
    for (auto & [pid, service] : _services) {
        auto val = fn(*service);
        if (val.show) {
            stm.add(u"count", val.indicator.in_err_count);

            ts::json::Value& v(pids.query(ts::UString::Decimal(pid, 0, true, u""), true));
            ts::json::Value& v2(v.query(name, true));
            v2.add(u"curr", val.indicator.in_err_count);
            if (val.indicator.show_value) {
                val.indicator.minMax.defineJson(v2);
            }
        }
    }
}

static void json_real(const char* name, const get_func& fn, ts::json::Value& stm, const std::map<ts::PID,  std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
{
    auto count = ::count(fn , _services);

    stm.add(ts::UString::FromUTF8(name), count);
    ts::json::Value& pids(stm.query(u"pids", true));
    json(ts::UString::FromUTF8(name).data(), fn, stm, pids, _services);
}

bool is_pes(const ts::TR101_290Analyzer::ServiceContext& ctx) {return ctx.type == ts::TR101_290Analyzer::ServiceContext::Assigned;}
bool is_pat(const ts::TR101_290Analyzer::ServiceContext& ctx) {return ctx.type == ts::TR101_290Analyzer::ServiceContext::Pat;}
bool is_pmt(const ts::TR101_290Analyzer::ServiceContext& ctx) {return ctx.type == ts::TR101_290Analyzer::ServiceContext::Pmt;}
bool is_table(const ts::TR101_290Analyzer::ServiceContext& ctx) {return ctx.type == ts::TR101_290Analyzer::ServiceContext::Pmt || ctx.type == ts::TR101_290Analyzer::ServiceContext::Pat;}

// ErrorState get_sync_byte_err(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.sy, true};}
ErrorState get_pat_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.PAT_error, is_pat(ctx)};}
ErrorState get_pat_error2(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.PAT_error_2,is_pat(ctx)};}
ErrorState get_cc_err(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.CC_error, true};}
ErrorState get_pmt_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.PMT_error, is_pmt(ctx)};}
ErrorState get_pmt_error2(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.PMT_error_2, is_pmt(ctx)};}
ErrorState get_pid_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.PID_error, !is_pmt(ctx) && !is_pat(ctx)};}
ErrorState get_transport_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.Transport_error, true};}
ErrorState get_crc_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.CRC_error, is_table(ctx)};}
ErrorState get_pcr_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.PCR_error, ctx.last_pcr_ts != ts::INVALID_PCR};}
ErrorState get_repetition_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.PCR_repetition_error, ctx.last_pcr_ts != ts::INVALID_PCR};}
ErrorState get_discontinuity_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.PCR_discontinuity_indicator_error, ctx.last_pcr_ts != ts::INVALID_PCR};}
ErrorState get_accuracy_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.PCR_accuracy_error, ctx.last_pcr_ts != ts::INVALID_PCR};}
ErrorState get_pts_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.PTS_error, is_pes(ctx)};}
ErrorState get_cat_error(const ts::TR101_290Analyzer::ServiceContext& ctx) {return {ctx.CAT_error, true};}

void ts::TR101_290Analyzer::report(std::ostream& stm, int& opt, Report& rep) const
{
    stm << "Priority 1 Errors:\n";
    // todo: TS_sync_loss may not be a valid test in IP-based systems and covered by Sync_byte_error.
    // print_real("TS_sync_loss", NULL, stm, _services);
    // print_real("Sync_byte_error", get_sync_byte_err, stm, _services);

    print_real(u"PAT_error", get_pat_error, stm, _services);
    print_real(u"PAT_error2", get_pat_error2, stm, _services);
    print_real(u"Continuity_count_error", get_cc_err, stm, _services);

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

void ts::TR101_290Analyzer::reportJSON(TR101_Options& opt, std::ostream& stm, const UString& title, Report& rep) const
{
    // JSON root.
    json::Object root;

    // Add user-supplied title.
    if (!title.empty()) {
        root.add(u"title", title);
    }

    json::Value& obj(root.query(u"tr101", true));

    // todo: TS_sync_loss may not be a valid test in IP-based systems and covered by Sync_byte_error.
    // print_real("TS_sync_loss", NULL, stm, _services);
    // print_real("Sync_byte_error", get_sync_byte_err, stm, _services);

    json_real("PAT_error", get_pat_error, obj, _services);
    json_real("PAT_error2", get_pat_error2, obj, _services);
    json_real("Continuity_count_error", get_cc_err, obj, _services);

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
        service->PAT_error.clear();
    }
}