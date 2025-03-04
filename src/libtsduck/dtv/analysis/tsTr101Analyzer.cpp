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

#define PCR_DISCONTINUITY_LIMIT long(100ul * ts::SYSTEM_CLOCK_FREQ /  1000)
#define PCR_REPETITION_LIMIT (100l * ts::SYSTEM_CLOCK_FREQ /  1000)
#define PCR_ACCURACY_LIMIT (500 * ts::SYSTEM_CLOCK_FREQ / int64_t(1e9))

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

void ts::TR101_290Analyzer::IntMinMax::pushSysClockFreq(int64_t val)
{
    const long ns = long(val * int64_t(1e9) / int64_t(SYSTEM_CLOCK_FREQ));
    pushNs(ns);
}

void ts::TR101_290Analyzer::IntMinMax::clear()
{
    curr = 0;
    min = LONG_MAX;
    max = LONG_MIN;
    count = 0;
}

ts::TR101_290Analyzer::Indicator::Indicator(UString _name, bool _show_value, bool _enabled, bool is_ms, uint64_t _value_timeout) :
    name(std::move(_name)),
    show_value(_show_value),
    enabled(_enabled),
    value_timeout(_value_timeout)
{
    minMax.is_ms = is_ms;
}

bool ts::TR101_290Analyzer::Indicator::timeout(bool timeout)
{
    if (timeout && !in_timeout)
        in_err_count++;

    in_timeout = timeout;
    return timeout;
}

bool ts::TR101_290Analyzer::Indicator::timeoutAfter(uint64_t now, uint64_t max_val)
{
    if (prev_ts != ts::INVALID_PCR && now - prev_ts > max_val) {
        return timeout(true);
    }
    return false;
}

bool ts::TR101_290Analyzer::Indicator::update(uint64_t now, bool in_error)
{
    prev_ts = now;

    if (in_error)
        in_err_count++;

    if (in_timeout)
        in_timeout = false;

    return in_error;
}

bool ts::TR101_290Analyzer::Indicator::update(uint64_t now, bool in_error, int64_t value)
{
    minMax.pushSysClockFreq(long(value));
    return update(now, in_error);
}

void ts::TR101_290Analyzer::Indicator::clear()
{
    in_err_count = 0;
    in_timeout = false;
}

bool ts::TR101_290Analyzer::Indicator::isEnabled() const
{
    return enabled;
}
bool ts::TR101_290Analyzer::Indicator::isOutdated(uint64_t now) const
{
    // We are never out of date if there was an error.
    return in_err_count == 0 && (now - prev_ts) > value_timeout;
}

void ts::TR101_290Analyzer::Indicator::setEnabled(bool enabled_)
{
    this->enabled = enabled_;
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
    PCR_error(u"PCR_error", true, false),
    PCR_repetition_error(u"PCR_repetition_error", true, false),
    PCR_discontinuity_indicator_error(u"PCR_discontinuity_indicator_error", true, false),
    PCR_accuracy_error(u"PCR_accuracy_error", true, false, false),
    PTS_error(u"PTS_error", true),
    CAT_error(u"CAT_error", false)
{
}

void ts::TR101_290Analyzer::ServiceContext::clear()
{
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
void ts::TR101_290Analyzer::ServiceContext::setType(ServiceContextType assignment)
{
    type = assignment;

    if (assignment == Pat) {
        PAT_error.setEnabled(true);
        PAT_error_2.setEnabled(true);

        PID_error.setEnabled(false);
        CRC_error.setEnabled(true);
    } else if (assignment == Pmt) {
        PMT_error.setEnabled(true);
        PMT_error_2.setEnabled(true);

        PID_error.setEnabled(false);
        CRC_error.setEnabled(true);
    } else {
        PAT_error.setEnabled(false);
        PAT_error_2.setEnabled(false);
        PMT_error.setEnabled(false);
        PMT_error_2.setEnabled(false);

        PID_error.setEnabled(true);
        CRC_error.setEnabled(type == Table);
        PTS_error.setEnabled(type != Table);
    }
}

ts::TR101_290Analyzer::TR101_290Analyzer(DuckContext& duck) :
    _duck(duck)
{
    auto ptr = std::make_shared<ServiceContext>(PID_PAT, ServiceContext::ServiceContextType::Pat);
    _services.insert(std::make_pair(PID_PAT, ptr));

    ptr = std::make_shared<ServiceContext>(PID_CAT, ServiceContext::ServiceContextType::Table);
    _services.insert(std::make_pair(PID_CAT, ptr));

    ptr = std::make_shared<ServiceContext>(PID_NIT, ServiceContext::ServiceContextType::Table);
    _services.insert(std::make_pair(PID_NIT, ptr));

    ptr = std::make_shared<ServiceContext>(PID_EIT, ServiceContext::ServiceContextType::Table);
    _services.insert(std::make_pair(PID_EIT, ptr));

    ptr = std::make_shared<ServiceContext>(PID_SDT, ServiceContext::ServiceContextType::Table);
    _services.insert(std::make_pair(PID_SDT, ptr));

    ptr = std::make_shared<ServiceContext>(PID_TOT, ServiceContext::ServiceContextType::Table);
    _services.insert(std::make_pair(PID_TOT, ptr));

    _demux.addPID(PID_PAT);
    _demux.addPID(PID_CAT);
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
            s2->setType(ServiceContext::Pmt);
            s2->pmt_service_id = service_id;
            demux.addPID(pid);
        }

        // Remove PMT assignments.
        for (auto & _service : _services) {
            if (_service.second->type == ServiceContext::Pmt && !pat.pmts.contains(uint16_t(_service.second->pmt_service_id))) {
                _service.second->setType(ServiceContext::Unassigned);
                demux.removePID(_service.first);
            }
        }
    } else if (table.tableId() == TID_PMT && service->type == ServiceContext::Pmt) {
        const auto pmt = PMT(_duck, table);

        // Ensure all PIDs are assigned to this service.
        for (const auto & [pid, stream] : pmt.streams) {
            const auto s2 = getService(pid);
            s2->setType(ServiceContext::Assigned);
            s2->pmt_service_id = service->pmt_service_id;
        }

        // Remove PIDs that are Assigned to this service ID.
        for (auto & [pid, service2] : _services) {
            if (service2->type == ServiceContext::Assigned && service2->pmt_service_id == service2->pmt_service_id) {
                if (!pmt.streams.contains(pid)) {
                    service2->setType(ServiceContext::Unassigned);
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
        service->PAT_error.update(currentTimestamp, section.tableId() != TID_PAT);
        service->PAT_error_2.update(currentTimestamp, section.tableId() != TID_PAT);

        if (section.tableId() == TID_PAT) {
            if (service->last_table_ts != INVALID_PCR) {
                // PID 0x0000 does not occur at least every 0,5 s
                auto diff = long(currentTimestamp - service->last_table_ts);
                service->PAT_error_2.update(currentTimestamp, false, diff);
                service->PAT_error_2.update(currentTimestamp, false, diff);
            }

            service->last_table_ts = currentTimestamp;
        }
    } else if (service->type == ServiceContext::Pmt && section.tableId() == TID_PMT) {
        if (service->last_table_ts != INVALID_PCR) {
            // PID 0x0000 does not occur at least every 0,5 s
            auto diff = long(currentTimestamp - service->last_table_ts);
            service->PMT_error.update(currentTimestamp, false, diff);
            service->PMT_error_2.update(currentTimestamp, false, diff);
        }

        service->last_table_ts = currentTimestamp;
    } else if (section.sourcePID() == PID_CAT) {
        if (section.tableId() == TID_CAT) {
            lastCatIndex = currentTimestamp;
        } else {
            // Section with table_id other than 0x01 (i.e. not a CAT) found on PID 0x0001
            service->CAT_error.update(currentTimestamp, true);
        }
    }
}

void ts::TR101_290Analyzer::handleInvalidSection(SectionDemux& demux, const DemuxedData& data, Section::Status status)
{
    auto service = getService(data.sourcePID());
    service->CRC_error.update(currentTimestamp, status == Section::Status::INV_CRC32);

    if (data.sourcePID() == PID_PAT) {
        // a PID 0x0000 does not contain a table_id 0x00 (i.e.a PAT)
        service->PAT_error.update(currentTimestamp, true);
        service->PAT_error_2.update(currentTimestamp, true);
    }
}

void ts::TR101_290Analyzer::processPacket(ServiceContext& ctx, const TSPacket& pkt, const TSPacketMetadata& mdata) const
{
    // Priority 1 Errors
    // todo: TS_sync_loss
    // todo: Sync_byte_error

    // Scrambling_control_field is not 00 for PID 0x0000
    if (pkt.getPID() == PID_PAT && pkt.getScrambling()) {
        ctx.PAT_error.update(currentTimestamp, true);
        ctx.PAT_error_2.update(currentTimestamp, true);
        info(ctx, ctx.PAT_error, u"Invalid scrambling bits (0b%d%d) on PAT pid.", pkt.getPID(), pkt.getScrambling() & 2, pkt.getScrambling() & 1);
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
            ctx.CC_error.update(currentTimestamp, true);
            info(ctx, ctx.CC_error, u"expected %d got %d on PAT pid.", expected_cc, pkt.getCC());
        }
    }
    ctx.last_cc = pkt.getCC();
    ctx.last_repeat = repeat;

    ctx.PID_error.update(currentTimestamp, false);

    // Priority 2 Errors
    if (ctx.Transport_error.update(currentTimestamp, pkt.getTEI() == true))
        info(ctx, ctx.Transport_error, u"Indicator was set.");
    // CRC_error in handleInvalidSection

    if (pkt.hasPCR()) {
        auto pcr_val = pkt.getPCR();

        // Enable PCR errors.
        ctx.PCR_error.setEnabled(true);
        ctx.PCR_accuracy_error.setEnabled(true);
        ctx.PCR_repetition_error.setEnabled(true);
        ctx.PCR_discontinuity_indicator_error.setEnabled(true);

        {
            // PCR discontinuity of more than 100 ms occurring
            // without specific indication.
            // Time interval between two consecutive PCR
            // values more than 100 ms
            if (ctx.last_pcr_ts != INVALID_PCR && !ctx.has_discontinuity) {
                auto error = int64_t(currentTimestamp - ctx.last_pcr_ts);
                if (ctx.PCR_error.update(currentTimestamp, error > PCR_DISCONTINUITY_LIMIT, error))
                    info(ctx, ctx.PCR_error, u"PCR not present for %d (%f sec) -- max %d (%f sec)", error, double(error)/double(SYSTEM_CLOCK_FREQ), PCR_DISCONTINUITY_LIMIT, double(PCR_DISCONTINUITY_LIMIT)/double(SYSTEM_CLOCK_FREQ));
            }
        }

        // Time interval between two consecutive PCR
        // values more than 100 ms
        ctx.PCR_repetition_error.update(currentTimestamp, false);

        {
            // The difference between two consecutive PCR
            // values(PCRi + 1 – PCRi) is outside the range of 0...100 ms
            // without the discontinuity_indicator set
            if (ctx.last_pcr_val != INVALID_PCR && !ctx.has_discontinuity) {
                auto error = int64_t(pcr_val) - int64_t(ctx.last_pcr_val);
                ctx.PCR_discontinuity_indicator_error.update(currentTimestamp, error > PCR_DISCONTINUITY_LIMIT || error < 0, error);
            }
        }

        // PCR accuracy of selected programme is not within ± 500 ns
        {
            if (ctx.last_pcr_val != INVALID_PCR && !ctx.has_discontinuity) {
                // This calculation is based on the calculation in tsplugin_pcrverify.cpp
                const int64_t epcr2 = ctx.last_pcr_val + (int64_t(currentTimestamp - ctx.last_pcr_ts) * PKT_SIZE_BITS * SYSTEM_CLOCK_FREQ) / bitrate.toInt();
                // Jitter = difference between actual and expected pcr2
                const int64_t jitter = int64_t(pcr_val) - epcr2;

                if (ctx.PCR_accuracy_error.update(currentTimestamp, jitter > PCR_ACCURACY_LIMIT, jitter)) {
                    info(ctx, ctx.PCR_accuracy_error, u"PCR jitter %'d (%'d ns)", jitter, (int64_t(1e9)*jitter) / SYSTEM_CLOCK_FREQ);
                }
            }
        }

        ctx.last_pcr_ts = currentTimestamp;
        ctx.last_pcr_val = pcr_val;
        ctx.has_discontinuity = false;
    }

    if (pkt.hasPTS()) {
        // PTS repetition period more than 700 ms
        if (ctx.last_pts_ts != INVALID_PCR) {
            auto error = int64_t(currentTimestamp - ctx.last_pts_ts);
            // todo: The limitation to 700 ms should not be applied to still pictures.
            ctx.PTS_error.update(currentTimestamp, error > PTS_REPETITION_INTERVAL, error);
        }
        ctx.last_pts_ts = currentTimestamp;
    }

    // Packets with transport_scrambling_control not 00
    // present, but no section with table_id = 0x01(i.e. a CAT) present
    ctx.CAT_error.update(currentTimestamp, pkt.getScrambling() && currentTimestamp - lastCatIndex > CAT_VALID_INTERVAL);
}

void ts::TR101_290Analyzer::processTimeouts(ServiceContext& ctx)
{
    // PID 0x0000 does not occur at least every 0,5 s
    ctx.PAT_error.timeoutAfter(currentTimestamp, PAT_INTERVAL);

    // Sections with table_id 0x00 do not occur at least
    // every 0, 5 s on PID 0x0000.
    ctx.PAT_error_2.timeoutAfter(currentTimestamp, PAT_INTERVAL);

    // Sections with table_id 0x02, (i.e. a PMT), do not
    // occur at least every 0, 5 s on the PID which is
    // referred to in the PAT
    ctx.PMT_error.timeoutAfter(currentTimestamp, PMT_INTERVAL);
    ctx.PMT_error_2.timeoutAfter(currentTimestamp, PMT_INTERVAL);

    ctx.PID_error.timeoutAfter(currentTimestamp, PMT_INTERVAL);

    // PCR discontinuity of more than 100 ms occurring
    // without specific indication.
    if (!ctx.has_discontinuity) {
        ctx.PCR_error.timeoutAfter(currentTimestamp, PCR_REPETITION_LIMIT);
    }

    // Time interval between two consecutive PCR
    // values more than 100 ms
    ctx.PCR_repetition_error.timeoutAfter(currentTimestamp, PCR_REPETITION_LIMIT);

    // PTS repetition period more than 700 ms
    // todo: NOTE 3: The limitation to 700 ms should not be applied to still pictures.
    ctx.PTS_error.timeoutAfter(currentTimestamp, PTS_REPETITION_INTERVAL);
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

void ts::TR101_290Analyzer::feedPacket(const TSPacket& packet, const TSPacketMetadata& mdata, const BitRate& new_bitrate)
{
    currentTimestamp = mdata.getInputTimeStamp().count();
    auto service = getService(packet.getPID());
    this->bitrate = new_bitrate;
    processTimeouts(*service);
    processPacket(*service, packet, mdata);
    _demux.feedPacket(packet);
}

static const char16_t* ERR = u"[ERR] ";
static const char16_t* OK  = u"[OK]  ";
static const char16_t* NA  = u"[N/A] ";

long ts::TR101_290Analyzer::count(Indicator ServiceContext::*indicator, const std::map<ts::PID, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
{
    long count = 0;
    for (auto & [pid, service] : _services) {
        auto val = (*service).*indicator;
        if (val.isEnabled())
            count += val.in_err_count;
    }
    return count;
}

void ts::TR101_290Analyzer::print(const char16_t* name, Indicator ServiceContext::*indicator,  std::ostream& stm, const std::map<ts::PID, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& services) const
{
    for (auto & [pid, service] : services) {
        auto val = (*service).*indicator;
        if (val.isEnabled()) {
            UString fmt;
            fmt.format(u"%X", service->pid);
            if (val.isOutdated(currentTimestamp)) {
                stm << u"\t" << NA << u"PID 0x" << fmt << u" ("<<service->pid<< u"): " << 0 << u"\n";

            } else {
                UString min_max;
                if (val.show_value)
                    min_max  = val.minMax.to_string();

                stm << u"\t" << (val.in_err_count == 0 ? OK : ERR) << u"PID 0x" << fmt << u" ("<<service->pid<< u"): " << val.in_err_count << u" " << min_max << u"\n";
            }
        }
    }
}

void ts::TR101_290Analyzer::json(const char16_t* name, Indicator ServiceContext::*indicator,  ts::json::Value& stm, ts::json::Value& pids, const std::map<ts::PID, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& _services)
{
    for (auto & [pid, service] : _services) {
        auto val = (*service).*indicator;
        if (val.isEnabled()) {
            stm.add(u"count", val.in_err_count);

            ts::json::Value& v(pids.query(ts::UString::Decimal(pid, 0, true, u""), true));
            ts::json::Value& v2(v.query(name, true));
            v2.add(u"curr", val.in_err_count);
            if (val.show_value) {
                val.minMax.defineJson(v2);
            }
        }
    }
}

void ts::TR101_290Analyzer::print_real(const char16_t* name, Indicator ServiceContext::*indicator, std::ostream& stm, const std::map<ts::PID, std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& services) const
{
    auto ret = count(indicator, services);

    stm << (ret == 0 ? OK : ERR) << u" " << name << u": " << ret << u"\n";
    print(name, indicator, stm, services);
}

void ts::TR101_290Analyzer::json_real(const char* name, Indicator ServiceContext::*indicator, ts::json::Value& stm, const std::map<ts::PID,  std::shared_ptr<ts::TR101_290Analyzer::ServiceContext>>& services)
{
    auto ret = count(indicator, services);

    stm.add(ts::UString::FromUTF8(name), ret);
    ts::json::Value& pids(stm.query(u"pids", true));
    json(ts::UString::FromUTF8(name).data(), indicator, stm, pids, services);
}

void ts::TR101_290Analyzer::report(std::ostream& stm, int& opt, Report& rep) const
{
    stm << "Priority 1 Errors:\n";
    // todo: TS_sync_loss may not be a valid test in IP-based systems and covered by Sync_byte_error.
    // print_real("TS_sync_loss", NULL, stm, _services);
    // print_real("Sync_byte_error", get_sync_byte_err, stm, _services);

    print_real(u"PAT_error", &ServiceContext::PAT_error, stm, _services);
    print_real(u"PAT_error2", &ServiceContext::PAT_error_2, stm, _services);
    print_real(u"Continuity_count_error", &ServiceContext::CC_error, stm, _services);

    print_real(u"PMT_error", &ServiceContext::PMT_error, stm, _services);
    print_real(u"PMT_error_2", &ServiceContext::PMT_error_2, stm, _services);

    print_real(u"PID_error", &ServiceContext::PID_error, stm, _services);

    stm << "\nPriority 2 Errors:\n";
    print_real(u"Transport_error", &ServiceContext::Transport_error, stm, _services);
    print_real(u"CRC_error", &ServiceContext::CRC_error, stm, _services);
    print_real(u"PCR_error", &ServiceContext::PCR_error, stm, _services);
    print_real(u"PCR_repetition_error", &ServiceContext::PCR_repetition_error, stm, _services);
    print_real(u"PCR_discontinuity_indicator_error", &ServiceContext::PCR_discontinuity_indicator_error, stm, _services);
    print_real(u"PCR_accuracy_error", &ServiceContext::PCR_accuracy_error, stm, _services);
    print_real(u"PTS_error", &ServiceContext::PTS_error, stm, _services);
    print_real(u"CAT_error", &ServiceContext::CAT_error, stm, _services);
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

    json_real("PAT_error", &ServiceContext::PAT_error, obj, _services);
    json_real("PAT_error2", &ServiceContext::PAT_error_2, obj, _services);
    json_real("Continuity_count_error", &ServiceContext::CC_error, obj, _services);

    json_real("PMT_error", &ServiceContext::PMT_error, obj, _services);
    json_real("PMT_error_2", &ServiceContext::PMT_error_2, obj, _services);

    json_real("PID_error", &ServiceContext::PID_error, obj, _services);

    json_real("Transport_error", &ServiceContext::Transport_error, obj, _services);
    json_real("CRC_error", &ServiceContext::CRC_error, obj, _services);
    json_real("PCR_error", &ServiceContext::PCR_error, obj, _services);
    json_real("PCR_repetition_error", &ServiceContext::PCR_repetition_error, obj, _services);
    json_real("PCR_discontinuity_indicator_error", &ServiceContext::PCR_discontinuity_indicator_error, obj, _services);
    json_real("PCR_accuracy_error", &ServiceContext::PCR_accuracy_error, obj, _services);
    json_real("PTS_error", &ServiceContext::PTS_error, obj, _services);
    json_real("CAT_error", &ServiceContext::CAT_error, obj, _services);

    opt.json.report(root, stm, rep);
}

void ts::TR101_290Analyzer::reset()
{
    for (auto & [_, service] : _services) {
        service->PAT_error.clear();
    }
}
