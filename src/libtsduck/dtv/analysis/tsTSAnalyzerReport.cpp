//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSAnalyzerReport.h"
#include "tsNames.h"
#include "tsjsonObject.h"
#include "tsAlgorithm.h"


//----------------------------------------------------------------------------
// Display layout
//----------------------------------------------------------------------------

#define DEF_WIDTH       79   // Default width.
#define DEF_TSBR_COL1   56   // TS bitrate, column 1.
#define DEF_TSBR_COL2   15   // TS bitrate, column 2.
#define DEF_SRV_COL1     6   // Service list, column 1 (id).
#define DEF_SRV_COL2    48   // Service list, column 2 (name).
#define DEF_SRV_COL3    15   // Service list, column 3 (bitrate).
#define DEF_PID_COL1     6   // PID list, column 1 (id).
#define DEF_PID_COL2    49   // PID list, column 2 (name).
#define DEF_PID_COL3    14   // PID list, column 3 (bitrate).

#define WIDE_WIDTH      94   // Wide display.
#define WIDE_TSBR_COL1  71   // TS bitrate, column 1.
#define WIDE_TSBR_COL2  15   // TS bitrate, column 2.
#define WIDE_SRV_COL1   15   // Service list, column 1 (id).
#define WIDE_SRV_COL2   54   // Service list, column 2 (name).
#define WIDE_SRV_COL3   15   // Service list, column 3 (bitrate).
#define WIDE_PID_COL1   14   // PID list, column 1 (id).
#define WIDE_PID_COL2   56   // PID list, column 2 (name).
#define WIDE_PID_COL3   14   // PID list, column 3 (bitrate).


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TSAnalyzerReport::TSAnalyzerReport(DuckContext& duck, const BitRate& bitrate_hint, BitRateConfidence bitrate_confidence) :
    TSAnalyzer(duck, bitrate_hint, bitrate_confidence)
{
}

ts::TSAnalyzerReport::~TSAnalyzerReport()
{
}


//----------------------------------------------------------------------------
// Set analysis options. Must be set before feeding the first packet.
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::setAnalysisOptions(const TSAnalyzerOptions& opt)
{
    setMinErrorCountBeforeSuspect(opt.suspect_min_error_count);
    setMaxConsecutiveSuspectCount(opt.suspect_max_consecutive);
}


//----------------------------------------------------------------------------
// General reporting method, using options
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::report(std::ostream& stm, TSAnalyzerOptions& opt, Report& rep)
{
    // Start with one-line reports
    size_t count = 0;

    if (opt.service_list) {
        // List of service ids
        std::vector<uint16_t> list;
        getServiceIds(list);
        for (size_t i = 0; i < list.size(); ++i) {
            stm << (count++ > 0 ? " " : "") << opt.prefix << int(list[i]);
        }
    }

    if (opt.pid_list) {
        // List of PIDs
        std::vector<PID> list;
        getPIDs (list);
        for (size_t i = 0; i < list.size(); ++i) {
            stm << (count++ > 0 ? " " : "") << opt.prefix << int(list[i]);
        }
    }

    if (opt.global_pid_list) {
        // List of global PIDs
        std::vector<PID> list;
        getGlobalPIDs(list);
        for (size_t i = 0; i < list.size(); ++i) {
            stm << (count++ > 0 ? " " : "") << opt.prefix << int(list[i]);
        }
    }

    if (opt.unreferenced_pid_list) {
        // List of unreferenced PIDs
        std::vector<PID> list;
        getUnreferencedPIDs(list);
        for (size_t i = 0; i < list.size(); ++i) {
            stm << (count++ > 0 ? " " : "") << opt.prefix << int(list[i]);
        }
    }

    if (opt.service_pid_list) {
        // List of PIDs for one service
        std::vector<PID> list;
        getPIDsOfService(list, opt.service_id);
        for (size_t i = 0; i < list.size(); ++i) {
            stm << (count++ > 0 ? " " : "") << opt.prefix << int(list[i]);
        }
    }

    if (opt.pes_pid_list) {
        // List of PIDs carrying PES packets
        std::vector<PID> list;
        getPIDsWithPES(list);
        for (size_t i = 0; i < list.size(); ++i) {
            stm << (count++ > 0 ? " " : "") << opt.prefix << int(list[i]);
        }
    }

    if (count > 0) {
        stm << std::endl;
    }

    // Then continue with grid reports.
    Grid grid(stm);
    grid.setLineWidth(opt.wide ? WIDE_WIDTH : DEF_WIDTH, 2);

    if (opt.ts_analysis) {
        reportTS(grid, opt.title);
    }
    if (opt.service_analysis) {
        reportServices(grid, opt.title);
    }
    if (opt.pid_analysis) {
        reportPIDs(grid, opt.title);
    }
    if (opt.table_analysis) {
        reportTables(grid, opt.title);
    }

    // Error reports in free format.
    if (opt.error_analysis) {
        reportErrors(stm, opt.title);
    }

    // Normalized report.
    if (opt.normalized) {
        reportNormalized(opt, stm, opt.title);
    }

    // JSON report.
    if (opt.json.useJSON()) {
        reportJSON(opt, stm, opt.title, rep);
    }
}


//----------------------------------------------------------------------------
// General reporting method, using the specified options.
//----------------------------------------------------------------------------

ts::UString ts::TSAnalyzerReport::reportToString(TSAnalyzerOptions& opt, Report& rep)
{
    std::stringstream stm(std::ios::out);
    report(stm, opt, rep);
    return UString::FromUTF8(stm.str());
}


//----------------------------------------------------------------------------
// Report a time stamp.
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportTimeStamp(Grid& grid, const UString& name, const Time& value) const
{
    grid.putLayout({{name, value == Time::Epoch ? u"Unknown" : value.format(Time::DATETIME)}});
}


//----------------------------------------------------------------------------
// Report global transport stream analysis
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportTS(Grid& grid, const UString& title)
{
    // Update the global statistics value if internal data were modified.
    recomputeStatistics();

    // Display additional values when the display is wide enough.
    const bool wide = grid.lineWidth() >= WIDE_WIDTH;

    grid.openTable();
    grid.putLine(u"TRANSPORT STREAM ANALYSIS REPORT", title);
    grid.section();

    grid.setLayout({grid.bothTruncateLeft(42, u'.'), grid.border(), grid.bothTruncateLeft(26, u'.')});
    grid.putLayout({{u"Transport Stream Id:", _ts_id.has_value() ? UString::Format(u"%d (0x%<X)", {*_ts_id}) : u"Unknown"},
                    {u"Services:", UString::Decimal(_services.size())}});
    grid.putLayout({{u"Bytes:", UString::Decimal(PKT_SIZE * _ts_pkt_cnt)},
                    {u"PID's: Total:", UString::Decimal(_pid_cnt)}});
    grid.putLayout({{u"TS packets:", UString::Decimal(_ts_pkt_cnt)},
                    {u"       Clear:", UString::Decimal(_pid_cnt - _scrambled_pid_cnt)}});
    grid.putLayout({{u"   With invalid sync:", UString::Decimal(_invalid_sync)},
                    {u"       Scrambled:", UString::Decimal(_scrambled_pid_cnt)}});
    grid.putLayout({{u"   With transport error:", UString::Decimal(_transport_errors)},
                    {u"       With PCR's:", UString::Decimal(_pcr_pid_cnt)}});
    grid.putLayout({{u"   Suspect and ignored:", UString::Decimal(_suspect_ignored)},
                    {u"       Unreferenced:", UString::Decimal(_unref_pid_cnt)}});
    grid.subSection();

    grid.setLayout({grid.bothTruncateLeft(wide ? WIDE_TSBR_COL1 : DEF_TSBR_COL1, u'.'),
                    grid.right(wide ? WIDE_TSBR_COL2 : DEF_TSBR_COL2)});
    grid.putLayout({{u"Transport stream bitrate, based on", u"188 bytes/pkt"},
                    {u"204 bytes/pkt"}});
    grid.putLayout({{u"User-specified:", _ts_user_bitrate == 0 ? u"None" : UString::Format(u"%'d b/s", {_ts_user_bitrate})},
                    {_ts_user_bitrate == 0 ? u"None" : UString::Format(u"%'d b/s", {ToBitrate204(_ts_user_bitrate)})}});
    grid.putLayout({{u"Estimated based on PCR's:", _ts_pcr_bitrate_188 == 0 ? u"Unknown" : UString::Format(u"%'d b/s", {_ts_pcr_bitrate_188})},
                    { _ts_pcr_bitrate_188 == 0 ? u"Unknown" : UString::Format(u"%'d b/s", {_ts_pcr_bitrate_204})}});
    grid.putLayout({{u"Selected reference bitrate:", _ts_bitrate == 0 ? u"Unknown" : UString::Format(u"%'d b/s", {_ts_bitrate})},
                    {_ts_bitrate == 0 ? u"None" : UString::Format(u"%'d b/s", {ToBitrate204(_ts_bitrate)})}});
    grid.subSection();

    grid.setLayout({grid.bothTruncateLeft(73, u'.')});
    grid.putLayout({{u"Broadcast time:", _duration == 0 ? u"Unknown" : UString::Format(u"%d sec (%d min %d sec)", {_duration / 1000, _duration / 60000, (_duration / 1000) % 60})}});
    if (_first_tdt != Time::Epoch || _first_tot != Time::Epoch || !_country_code.empty()) {
        // This looks like a DVB stream.
        reportTimeStamp(grid, u"First TDT UTC time stamp:", _first_tdt);
        reportTimeStamp(grid, u"Last TDT UTC time stamp:", _last_tdt);
        reportTimeStamp(grid, u"First TOT local time stamp:", _first_tot);
        reportTimeStamp(grid, u"Last TOT local time stamp:", _last_tot);
        grid.putLayout({{u"TOT country code:", _country_code.empty() ? u" Unknown" : _country_code}});
    }
    if (_first_stt != Time::Epoch) {
        // This looks like an ATSC stream.
        reportTimeStamp(grid, u"First STT UTC time stamp:", _first_stt);
        reportTimeStamp(grid, u"Last STT UTC time stamp:", _last_stt);
    }
    grid.subSection();

    // Display list of services

    grid.setLayout({wide ? grid.both(WIDE_SRV_COL1) : grid.right(DEF_SRV_COL1),
                    grid.bothTruncateLeft(wide ? WIDE_SRV_COL2 : DEF_SRV_COL2),
                    grid.right(wide ? WIDE_SRV_COL3 : DEF_SRV_COL3)});
    grid.putLayout({{u"Srv Id", u""}, {u"Service Name", u"Access"}, {u"Bitrate"}});
    grid.setLayout({wide ? grid.both(WIDE_SRV_COL1) : grid.right(DEF_SRV_COL1),
                    grid.bothTruncateLeft(wide ? WIDE_SRV_COL2 : DEF_SRV_COL2, u'.'),
                    grid.right(wide ? WIDE_SRV_COL3 : DEF_SRV_COL3)});

    for (const auto& it : _services) {
        const ServiceContext& sv(*it.second);
        // Not that the decimal service id is always built but ignored when the layout of the first column contains only one field.
        grid.putLayout({{UString::Format(u"0x%X", {sv.service_id}), UString::Format(u"(%d)", {sv.service_id})},
                        {sv.getName(), sv.scrambled_pid_cnt > 0 ? u"S" : u"C"},
                        {sv.bitrate == 0 ? u"Unknown" : UString::Format(u"%'d b/s", {sv.bitrate})}});
    }

    grid.putLine();
    grid.putLine(u"Note 1: C=Clear, S=Scrambled");
    grid.putMultiLine(u"Note 2: Unless specified otherwise, bitrates are based on 188 bytes/pkt");

    grid.closeTable();
}


//----------------------------------------------------------------------------
// Display header of a service PID list
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportServiceHeader(Grid& grid, const UString& usage, bool scrambled, const BitRate& bitrate, const BitRate& ts_bitrate, bool wide) const
{
    grid.subSection();
    grid.setLayout({wide ? grid.both(WIDE_PID_COL1) : grid.right(DEF_PID_COL1),
                    grid.bothTruncateLeft(wide ? WIDE_PID_COL2 : DEF_PID_COL2),
                    grid.right(wide ? WIDE_PID_COL3 : DEF_PID_COL3)});
    grid.putLayout({{u"PID", u""}, {u"Usage", u"Access "}, {u"Bitrate"}});
    grid.setLayout({wide ? grid.both(WIDE_PID_COL1) : grid.right(DEF_PID_COL1),
                    grid.bothTruncateLeft(wide ? WIDE_PID_COL2 : DEF_PID_COL2, u'.'),
                    grid.right(wide ? WIDE_PID_COL3 : DEF_PID_COL3)});
    reportServiceSubtotal(grid, u"Total", usage, scrambled, bitrate, ts_bitrate, wide);
}


//----------------------------------------------------------------------------
// Display one line of a subtotal
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportServiceSubtotal(Grid& grid, const UString& header, const UString& usage, bool scrambled, const BitRate& bitrate, const BitRate& ts_bitrate, bool wide) const
{
    grid.putLayout({{header, u""}, {usage, scrambled ? u"S " : u"C "}, {ts_bitrate == 0 ? u"Unknown" : UString::Format(u"%'d b/s", {bitrate})}});
}


//----------------------------------------------------------------------------
// Display one line of a service PID list
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportServicePID(Grid& grid, const PIDContext& pc) const
{
    const UString access{pc.scrambled ? u'S' : u'C', pc.services.size() > 1 ? u'+' : u' '};

    // Build a description string for the PID.
    UString description(pc.fullDescription(true));
    if (!pc.ssu_oui.empty()) {
        bool first = true;
        for (auto oui : pc.ssu_oui) {
            description += first ? u" (SSU " : u", ";
            description += NameFromOUI(oui);
            first = false;
        }
        description += u")";
    }

    // PID line. Not that the decimal PID is always built but ignored when the layout
    // of the first column contains only one field (the hexa value).
    grid.putLayout({{UString::Format(u"0x%X", {pc.pid}), UString::Format(u"(%d)", {pc.pid})},
                    {description, access},
                    {_ts_bitrate == 0 ? u"Unknown" : UString::Format(u"%'d b/s", {pc.bitrate})}});
}


//----------------------------------------------------------------------------
// Report services analysis
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportServices(Grid& grid, const UString& title)
{
    // Update the global statistics value if internal data were modified.
    recomputeStatistics();

    // Display additional values when the display is wide enough.
    const bool wide = grid.lineWidth() >= WIDE_WIDTH;

    grid.openTable();
    grid.putLine(u"SERVICES ANALYSIS REPORT", title);

    // Display global pids
    grid.section();
    grid.putLine(u"Global PID's");
    grid.putLine(UString::Format(u"TS packets: %'d, PID's: %d (clear: %d, scrambled: %d)", {_global_pkt_cnt, _global_pid_cnt, _global_pid_cnt - _global_scr_pids, _global_scr_pids}));
    reportServiceHeader(grid, u"Global PID's", _global_scr_pids > 0, _global_bitrate, _ts_bitrate, wide);
    reportServiceSubtotal(grid, wide ? u"Subtotal" : u"Subt.", u"Global PSI/SI PID's (0x00-0x1F)", _psisi_scr_pids > 0, _psisi_bitrate, _ts_bitrate, wide);

    for (const auto& it : _pids) {
        const PIDContext& pc(*it.second);
        if (pc.referenced && pc.services.empty() && (pc.ts_pkt_cnt != 0 || !pc.optional)) {
            reportServicePID(grid, pc);
        }
    }

    // Display unreferenced pids
    if (_unref_pid_cnt > 0) {
        grid.section();
        grid.putLine(u"Unreferenced PID's");
        grid.putLine(UString::Format(u"TS packets: %'d, PID's: %d (clear: %d, scrambled: %d)", {_unref_pkt_cnt, _unref_pid_cnt, _unref_pid_cnt - _unref_scr_pids, _unref_scr_pids}));
        reportServiceHeader(grid, u"Unreferenced PID's", _unref_scr_pids > 0, _unref_bitrate, _ts_bitrate, wide);

        for (const auto& it : _pids) {
            const PIDContext& pc(*it.second);
            if (!pc.referenced && (pc.ts_pkt_cnt != 0 || !pc.optional)) {
                reportServicePID(grid, pc);
            }
        }
    }

    // Display list of services
    for (const auto& it : _services) {

        const ServiceContext& sv(*it.second);
        grid.section();
        grid.putLine(UString::Format(u"Service: 0x%X (%<d)", {sv.service_id}) +
                     (_ts_id.has_value() ? UString::Format(u", TS: 0x%X (%<d)", {*_ts_id}) : UString()) +
                     (sv.orig_netw_id.has_value() ? UString::Format(u", Original Netw: 0x%X (%<d)", {*sv.orig_netw_id}) : UString()));
        grid.putLine(UString::Format(u"Service name: %s, provider: %s", {sv.getName(), sv.getProvider()}) +
                     (sv.lcn.has_value() ? UString::Format(u", LCN: %d", {*sv.lcn}) : UString()) +
                     (sv.hidden ? u" (hidden)" : u""));
        grid.putLine(u"Service type: " + names::ServiceType(sv.service_type, NamesFlags::FIRST));
        grid.putLine(UString::Format(u"TS packets: %'d, PID's: %d (clear: %d, scrambled: %d)", {sv.ts_pkt_cnt, sv.pid_cnt, sv.pid_cnt - sv.scrambled_pid_cnt, sv.scrambled_pid_cnt}));
        grid.putLine(u"PMT PID: " +
                     (sv.pmt_pid == 0 || sv.pmt_pid == PID_NULL ? u"Unknown in PAT" : UString::Format(u"0x%X (%d)", {sv.pmt_pid, sv.pmt_pid})) +
                     u", PCR PID: " +
                     (sv.pcr_pid == 0 || sv.pcr_pid == PID_NULL ? u"None" : UString::Format(u"0x%X (%<d)", {sv.pcr_pid})));

        // Display all PID's of this service
        reportServiceHeader(grid, names::ServiceType(sv.service_type), sv.scrambled_pid_cnt > 0, sv.bitrate, _ts_bitrate, wide);
        for (const auto& pid_it : _pids) {
            const PIDContext& pc(*pid_it.second);
            if (Contains(pc.services, sv.service_id)) {
                reportServicePID(grid, pc);
            }
        }

        grid.setLayout({grid.both(wide?14:6), grid.bothTruncateLeft(wide?56:49), grid.right(14)});

        grid.putLayout({{u""}, {u"(C=Clear, S=Scrambled, +=Shared)"}, {u""}});
    }

    grid.closeTable();
}


//----------------------------------------------------------------------------
// Print list of services a PID belongs to.
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportServicesForPID(Grid& grid, const PIDContext& pc) const
{
    for (const auto& serv_id : pc.services) {
        auto serv_it = _services.find(serv_id);
        grid.putLine(UString::Format(u"Service: 0x%X (%d) %s", {serv_id, serv_id, serv_it == _services.end() ? UString() : serv_it->second->getName()}));
    }
}


//----------------------------------------------------------------------------
// Report PID's analysis.
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportPIDs(Grid& grid, const UString& title)
{
    // Update the global statistics value if internal data were modified.
    recomputeStatistics();

    grid.openTable();
    grid.putLine(u"PIDS ANALYSIS REPORT", title);

    // Loop on all analyzed PID's.
    for (const auto& it : _pids) {

        // Get PID description, ignore if no packet was found.
        // A PID can be declared, in a PMT for instance, but has no traffic on it.
        const PIDContext& pc(*it.second);
        if (pc.ts_pkt_cnt == 0) {
            continue;
        }

        // Type of PID.
        UString pid_type;
        if (pc.services.size() == 1) {
            pid_type = u"Single Service PID";
        }
        else if (pc.services.size() > 1) {
            pid_type = u"Shared PID";
        }
        else if (pc.referenced) {
            pid_type = u"Global PID";
        }
        else {
            pid_type = u"Unreferenced PID";
        }

        // The crypto-period is measured in number of TS packets, translate it.
        UString crypto_period;
        if (!pc.scrambled || pc.crypto_period == 0) {
            crypto_period = u"Unknown";
        }
        else if (_ts_bitrate == 0) {
            crypto_period = UString::Format(u"%d pkt", {pc.crypto_period});
        }
        else {
            crypto_period = UString::Format(u"%d sec", {((pc.crypto_period * PKT_SIZE_BITS) / _ts_bitrate).toInt()});
        }

        // Header lines
        grid.section();
        grid.putLine(UString::Format(u"PID: 0x%X (%d)", {pc.pid, pc.pid}), pc.fullDescription(false), false);

        // Type of PES data, if available
        if (pc.same_stream_id) {
            grid.putLine(u"PES stream id: " + NameFromDTV(u"pes.stream_id", pc.pes_stream_id, NamesFlags::FIRST));
        }

        // Audio/video attributes
        for (auto& attr : pc.attributes) {
            if (!attr.empty()) {
                grid.putLine(attr);
            }
        }

        // List of services to which the PID belongs to
        reportServicesForPID(grid, pc);

        // List of System Software Update OUI's on this PID
        for (auto oui : pc.ssu_oui) {
            grid.putLine(u"SSU OUI: " + NameFromOUI(oui, NamesFlags::FIRST));
        }
        grid.subSection();

        // 3-columns output.
        grid.setLayout({grid.left(24), grid.left(24), grid.left(21)});
        grid.putLayout({{pid_type}, {u"Transport:"}, {u"Discontinuities:"}});

        grid.setLayout({grid.bothTruncateLeft(24, u'.'), grid.bothTruncateLeft(24, u'.'), grid.bothTruncateLeft(21, u'.')});
        grid.putLayout({{u"Bitrate:", _ts_bitrate == 0 ? u"Unknown" : UString::Format(u"%'d b/s", {pc.bitrate})},
                        {u"Packets:", UString::Decimal(pc.ts_pkt_cnt)},
                        {u"Expected:", UString::Decimal(pc.exp_discont)}});
        grid.putLayout({{u"Access:", pc.scrambled ? u"Scrambled" : u"Clear"},
                        {u"Adapt.F.:", UString::Decimal(pc.ts_af_cnt)},
                        {u"Unexpect:", UString::Decimal(pc.unexp_discont)}});

        grid.setLayout({grid.bothTruncateLeft(24, u'.'), grid.bothTruncateLeft(24, u'.'), grid.left(21)});
        grid.putLayout({{pc.scrambled ? u"Crypto-Per:" : u"", pc.scrambled ? crypto_period : u""},
                        {u"Duplicated:", UString::Decimal(pc.duplicated)},
                        {pc.carry_pes ? u"PES:" : u"Sections:"}});

        grid.setLayout({grid.bothTruncateLeft(24, u'.'), grid.bothTruncateLeft(24, u'.'), grid.bothTruncateLeft(21, u'.')});
        grid.putLayout({{pc.scrambled ? u"Inv.scramb.:" : u"", pc.scrambled ? UString::Decimal(pc.inv_ts_sc_cnt) : u""},
                        {pc.ts_pcr_bitrate > 0 ? u"TSrate:" : u"", pc.ts_pcr_bitrate > 0 ? UString::Format(u"%'d b/s", {pc.ts_pcr_bitrate}) : u""},
                        {pc.carry_pes ? u"Packets:" : u"Unit start:", UString::Decimal(pc.carry_pes ? pc.pl_start_cnt : pc.unit_start_cnt)}});

        if (pc.ts_pcr_bitrate > 0 || pc.carry_pes) {
            grid.putLayout({{u""},
                            {u""},
                            {pc.carry_pes ? u"Inv.Start:" : u"", pc.carry_pes ? UString::Decimal(pc.inv_pes_start) : u""}});
        }

        const bool has_pcr = pc.first_pcr != INVALID_PCR;
        const bool has_pts = pc.first_pts != INVALID_PTS;
        const bool has_dts = pc.first_dts != INVALID_DTS;

        if (has_pcr || has_pts || has_dts) {
            grid.setLayout({grid.left(24), grid.left(24), grid.left(21)});
            grid.putLayout({{u"Clock values range:"}});
            grid.setLayout({grid.bothTruncateLeft(24, u'.'), grid.bothTruncateLeft(24, u'.'), grid.bothTruncateLeft(21, u'.')});
            grid.putLayout({{has_pcr ? u"PCR:" : u"", has_pcr ? UString::Decimal(pc.pcr_cnt) : u""},
                            {has_pts ? u"PTS:" : u"", has_pts ? UString::Decimal(pc.pts_cnt) : u""},
                            {has_dts ? u"DTS:" : u"", has_dts ? UString::Decimal(pc.dts_cnt) : u""}});
            grid.putLayout({{has_pcr ? u"from" : u"", has_pcr ? UString::Decimal(pc.first_pcr) : u""},
                            {has_pts ? u"from" : u"", has_pts ? UString::Decimal(pc.first_pts) : u""},
                            {has_dts ? u"from" : u"", has_dts ? UString::Decimal(pc.first_dts) : u""}});
            grid.putLayout({{has_pcr ? u"to" : u"", has_pcr ? UString::Decimal(pc.last_pcr) : u""},
                            {has_pts ? u"to" : u"", has_pts ? UString::Decimal(pc.last_pts) : u""},
                            {has_dts ? u"to" : u"", has_dts ? UString::Decimal(pc.last_dts) : u""}});
            grid.putLayout({{has_pcr ? u"Leaps:" : u"", has_pcr ? UString::Decimal(pc.pcr_leap_cnt) : u""},
                            {has_pts ? u"Leaps:" : u"", has_pts ? UString::Decimal(pc.pts_leap_cnt) : u""},
                            {has_dts ? u"Leaps:" : u"", has_dts ? UString::Decimal(pc.dts_leap_cnt) : u""}});
        }
    }

    grid.closeTable();
}


//----------------------------------------------------------------------------
// Report tables analysis
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportTables(Grid& grid, const UString& title)
{
    // Update the global statistics value if internal data were modified.
    recomputeStatistics();

    grid.openTable();
    grid.putLine(u"TABLES & SECTIONS ANALYSIS REPORT", title);

    // Loop on all PID's
    for (const auto& pid_it : _pids) {

        // Get PID description, ignore if PID without sections
        const PIDContext& pc(*pid_it.second);
        if (pc.sections.empty()) {
            continue;
        }

        // Header line: PID
        grid.section();
        grid.putLine(UString::Format(u"PID: 0x%X (%d)", {pc.pid, pc.pid}), pc.fullDescription(false), false);

        // Header lines: list of services to which the PID belongs to
        reportServicesForPID(grid, pc);

        // Loop on all tables on this PID
        for (const auto& sec_it : pc.sections) {
            const ETIDContext& etc(*sec_it.second);
            const TID tid = etc.etid.tid();
            const bool isShort = etc.etid.isShortSection();

            // Repetition rates are displayed in ms if the TS bitrate is known, in packets otherwise.
            const UChar* unit = nullptr;
            uint64_t rep, min_rep, max_rep;
            if (_ts_bitrate != 0) {
                unit = u" ms";
                rep = PacketInterval(_ts_bitrate, etc.repetition_ts);
                min_rep = PacketInterval(_ts_bitrate, etc.min_repetition_ts);
                max_rep = PacketInterval(_ts_bitrate, etc.max_repetition_ts);
            }
            else {
                unit = u" pkt";
                rep = etc.repetition_ts;
                min_rep = etc.min_repetition_ts;
                max_rep = etc.max_repetition_ts;
            }

            // Version description
            const size_t version_count = etc.versions.count();
            const UChar* version_title = nullptr;
            UString version_list;
            bool first = true;
            for (size_t i = 0; i < etc.versions.size(); ++i) {
                if (etc.versions.test(i)) {
                    version_list.append(UString::Format(u"%s%d", {first ? u"" : u", ", i}));
                    first = false;
                }
            }
            if (version_count == 0) {
                version_title = u"";
            }
            else if (version_count == 1) {
                version_title = u"Version:";
            }
            else {
                version_title = u"Versions:";
            }

            // Header line: TID
            grid.subSection();
            grid.putLine(names::TID(_duck, tid, pc.cas_id, NamesFlags::BOTH_FIRST) +
                         (isShort ? u"" : UString::Format(u", TID ext: 0x%X (%d)", {etc.etid.tidExt(), etc.etid.tidExt()})));

            // 4-columns output, first column remains empty.
            grid.setLayout({grid.left(2), grid.bothTruncateLeft(25, u'.'), grid.bothTruncateLeft(23, u'.'), grid.bothTruncateLeft(17, u'.')});
            grid.putLayout({{u""},
                            {u"Repetition:", UString::Format(u"%d %s", {rep, unit})},
                            {u"Section cnt:", UString::Decimal(etc.section_count)},
                            {version_count <= 1 ? u"": u"First version:", version_count <= 1 ? u"": UString::Decimal(etc.first_version)}});
            grid.putLayout({{u""},
                            {u"Min repet.:", UString::Format(u"%d %s", {min_rep, unit})},
                            {isShort ? u"" : u"Table cnt:", isShort ? u"" : UString::Decimal(etc.table_count)},
                            {version_count <= 1 ? u"": u"Last version:", version_count <= 1 ? u"": UString::Decimal(etc.last_version)}});
            if (version_count > 3) {
                // Merge last two columns.
                grid.setLayout({grid.left(2), grid.bothTruncateLeft(25, u'.'), grid.bothTruncateLeft(42, u'.')});
            }
            grid.putLayout({{u""},
                            {u"Max repet.:", UString::Format(u"%d %s", {max_rep, unit})},
                            {version_title, version_list},
                            {u"", u""}});
        }
    }

    grid.closeTable();
}


//----------------------------------------------------------------------------
// This methods displays an error report
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportErrors(std::ostream& stm, const UString& title)
{
    int error_count = 0;
    const uint16_t tsid = _ts_id.value_or(0xFFFF);

    // Update the global statistics value if internal data were modified.
    recomputeStatistics();

    // Header
    stm << "TITLE: ERROR ANALYSIS REPORT" << std::endl;
    if (!title.empty()) {
        stm << "TITLE: " << title << std::endl;
    }
    if (_ts_id.has_value()) {
        stm << UString::Format(u"INFO: Transport Stream Identifier: %d (0x%<X)", {tsid}) << std::endl;
    }

    // Report global errors
    if (_invalid_sync > 0) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%<X: TS packets with invalid sync byte: %d", {tsid, _invalid_sync}) << std::endl;
    }
    if (_transport_errors > 0) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%<X: TS packets with transport error indicator: %d", {tsid, _transport_errors}) << std::endl;
    }
    if (_suspect_ignored > 0) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%<X: suspect TS packets, ignored: %d", {tsid, _suspect_ignored}) << std::endl;
    }
    if (_unref_pid_cnt > 0) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%<X: Unreferenced PID's: %d", {tsid, _unref_pid_cnt}) << std::endl;
    }

    // Report missing standard DVB tables
    if (!_tid_present[TID_PAT]) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%<X: No PAT", {tsid}) << std::endl;
    }
    if (_scrambled_pid_cnt > 0 && !_tid_present[TID_CAT]) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%<X: No CAT (%d scrambled PID's)", {tsid, _scrambled_pid_cnt}) << std::endl;
    }
    if (!_tid_present[TID_SDT_ACT]) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%<X: No SDT Actual", {tsid}) << std::endl;
    }
    if (!_tid_present[TID_BAT]) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%<X: No BAT", {tsid}) << std::endl;
    }
    if (!_tid_present[TID_TDT]) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%<X: No TDT", {tsid}) << std::endl;
    }
    if (!_tid_present[TID_TOT]) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%<X: No TOT", {tsid}) << std::endl;
    }

    // Report error per PID
    for (auto& pid_it : _pids) {
        const PIDContext& pc(*pid_it.second);
        if (pc.exp_discont > 0) {
            error_count++;
            stm << UString::Format(u"PID:%d:0x%<X: Discontinuities (expected): %d", {pc.pid, pc.exp_discont}) << std::endl;
        }
        if (pc.unexp_discont > 0) {
            error_count++;
            stm << UString::Format(u"PID:%d:0x%<X: Discontinuities (unexpected): %d", {pc.pid, pc.unexp_discont}) << std::endl;
        }
        if (pc.duplicated > 0) {
            error_count++;
            stm << UString::Format(u"PID:%d:0x%<X: Duplicated TS packets: %d", {pc.pid, pc.duplicated}) << std::endl;
        }
        if (pc.inv_ts_sc_cnt > 0) {
            error_count++;
            stm << UString::Format(u"PID:%d:0x%<X: Invalid scrambling control values: %d", {pc.pid, pc.inv_ts_sc_cnt}) << std::endl;
        }
        if (pc.carry_pes && pc.inv_pes_start > 0) {
            error_count++;
            stm << UString::Format(u"PID:%d:0x%<X: Invalid PES header start codes: %d", {pc.pid, pc.inv_pes_start}) << std::endl;
        }
        if (pc.carry_pes && pc.inv_pes > 0) {
            error_count++;
            stm << UString::Format(u"PID:%d:0x%<X: Invalid PES packets: %d", {pc.pid, pc.inv_pes}) << std::endl;
        }
        if (pc.carry_section && pc.inv_sections > 0) {
            error_count++;
            stm << UString::Format(u"PID:%d:0x%<X: Invalid sections: %d", {pc.pid, pc.inv_sections}) << std::endl;
        }
        if (pc.is_pmt_pid && pc.pmt_cnt == 0) {
            assert(!pc.services.empty());
            int service_id(*(pc.services.begin()));
            error_count++;
            stm << UString::Format(u"PID:%d:0x%<X: No PMT (PMT PID of service %d, 0x%<X)", {pc.pid, service_id}) << std::endl;
        }
        if (pc.is_pcr_pid && pc.pcr_cnt == 0) {
            error_count++;
            stm << UString::Format(u"PID:%d:0x%<X: No PCR, PCR PID of service%s", {pc.pid, pc.services.size() > 1 ? u"s" : u""});
            bool first = true;
            for (auto& srv : pc.services) {
                stm << (first ? "" : ",") << UString::Format(u" %d (0x%<X)", {srv});
                first = false;
            }
            stm << std::endl;
        }
    }

    // Summary
    stm << "SUMMARY: Error count: " << error_count << std::endl;
}


//----------------------------------------------------------------------------
// This static method displays a normalized time value.
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportNormalizedTime(std::ostream& stm, const Time& time, const char* type, const UString& country)
{
    if (time != Time::Epoch) {
        const Time::Fields f(time);
        stm << type << ":"
            << UString::Format(u"date=%02d/%02d/%04d:", {f.day, f.month, f.year})
            << UString::Format(u"time=%02dh%02dm%02ds:", {f.hour, f.minute, f.second})
            << "secondsince2000=" << ((time - Time(2000, 1, 1, 0, 0, 0)) / MilliSecPerSec) << ":";
        if (!country.empty()) {
            stm << "country=" << country << ":";
        }
        stm << std::endl;
    }
}


//----------------------------------------------------------------------------
// This method displays a normalized report.
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportNormalized(TSAnalyzerOptions& opt, std::ostream& stm, const UString& title)
{
    // Update the global statistics value if internal data were modified.
    recomputeStatistics();

    // Print one line with user-supplied title
    stm << "title:" << title << std::endl;

    // Print one line with transport stream description
    stm << "ts:";
    if (_ts_id.has_value()) {
        stm << "id=" << *_ts_id << ":";
    }
    stm << "services=" << _services.size() << ":"
        << "clearservices=" << (_services.size() - _scrambled_services_cnt) << ":"
        << "scrambledservices=" << _scrambled_services_cnt << ":"
        << "pids=" << _pid_cnt << ":"
        << "clearpids=" << (_pid_cnt - _scrambled_pid_cnt) << ":"
        << "scrambledpids=" << _scrambled_pid_cnt << ":"
        << "pcrpids=" << _pcr_pid_cnt << ":"
        << "unreferencedpids=" << _unref_pid_cnt << ":"
        << "packets=" << _ts_pkt_cnt << ":"
        << "invalidsyncs=" << _invalid_sync << ":"
        << "transporterrors=" << _transport_errors << ":"
        << "suspectignored=" << _suspect_ignored << ":"
        << "bytes=" << (PKT_SIZE * _ts_pkt_cnt) << ":"
        << "bitrate=" << _ts_bitrate.toInt() << ":"
        << "bitrate204=" << ToBitrate204(_ts_bitrate).toInt() << ":"
        << "userbitrate=" << _ts_user_bitrate.toInt() << ":"
        << "userbitrate204=" << ToBitrate204(_ts_user_bitrate).toInt() << ":"
        << "pcrbitrate=" << _ts_pcr_bitrate_188.toInt() << ":"
        << "pcrbitrate204=" << _ts_pcr_bitrate_204.toInt() << ":"
        << "duration=" << (_duration / 1000) << ":";
    if (!_country_code.empty()) {
        stm << "country=" << _country_code << ":";
    }
    stm << std::endl;

    // Print lines for first and last UTC and local time
    reportNormalizedTime(stm, _first_tdt, "time:utc:tdt:first");
    reportNormalizedTime(stm, _last_tdt, "time:utc:tdt:last");
    reportNormalizedTime(stm, _first_tot, "time:local:tot:first", _country_code);
    reportNormalizedTime(stm, _last_tot, "time:local:tot:last", _country_code);
    if (!opt.deterministic) {
        reportNormalizedTime(stm, _first_utc, "time:utc:system:first");
        reportNormalizedTime(stm, _last_utc, "time:utc:system:last");
        reportNormalizedTime(stm, _first_local, "time:local:system:first");
        reportNormalizedTime(stm, _last_local, "time:local:system:last");
    }

    // Print one line for global PIDs
    stm << "global:"
        << "pids=" << _global_pid_cnt << ":"
        << "clearpids=" << (_global_pid_cnt - _global_scr_pids) << ":"
        << "scrambledpids=" << _global_scr_pids << ":"
        << "packets=" << _global_pkt_cnt << ":"
        << "bitrate=" << _global_bitrate.toInt() << ":"
        << "bitrate204=" << ToBitrate204(_global_bitrate).toInt() << ":"
        << "access=" << (_global_scr_pids > 0 ? "scrambled" : "clear") << ":"
        << "pidlist=";
    bool first = true;
    for (const auto& it : _pids) {
        const PIDContext& pc(*it.second);
        if (pc.referenced && pc.services.size() == 0 && (pc.ts_pkt_cnt != 0 || !pc.optional)) {
            stm << (first ? "" : ",") << pc.pid;
            first = false;
        }
    }
    stm << ":" << std::endl;

    // Print one line for unreferenced PIDs
    stm << "unreferenced:"
        << "pids=" << _unref_pid_cnt << ":"
        << "clearpids=" << (_unref_pid_cnt - _unref_scr_pids) << ":"
        << "scrambledpids=" << _unref_scr_pids << ":"
        << "packets=" << _unref_pkt_cnt << ":"
        << "bitrate=" << _unref_bitrate.toInt() << ":"
        << "bitrate204=" << ToBitrate204(_unref_bitrate).toInt() << ":"
        << "access=" << (_unref_scr_pids > 0 ? "scrambled" : "clear") << ":"
        << "pidlist=";
    first = true;
    for (const auto& it : _pids) {
        const PIDContext& pc (*it.second);
        if (!pc.referenced && (pc.ts_pkt_cnt != 0 || !pc.optional)) {
            stm << (first ? "" : ",") << pc.pid;
            first = false;
        }
    }
    stm << ":" << std::endl;

    // Print one line per service
    for (const auto& it : _services) {
        const ServiceContext& sv(*it.second);
        stm << "service:id=" << sv.service_id;
        if (_ts_id.has_value()) {
            stm << ":tsid=" << *_ts_id;
        }
        if (sv.orig_netw_id.has_value()) {
            stm << ":orignetwid=" << *sv.orig_netw_id;
        }
        if (sv.lcn.has_value()) {
            stm << ":lcn=" << *sv.lcn;
        }
        stm << ":access=" << (sv.scrambled_pid_cnt > 0 ? "scrambled" : "clear")
            << ":pids=" << sv.pid_cnt
            << ":clearpids=" << (sv.pid_cnt - sv.scrambled_pid_cnt)
            << ":scrambledpids=" << sv.scrambled_pid_cnt
            << ":packets=" << sv.ts_pkt_cnt
            << ":bitrate=" << sv.bitrate.toInt()
            << ":bitrate204=" << ToBitrate204(sv.bitrate).toInt()
            << ":servtype=" << int(sv.service_type);
        if (sv.hidden) {
            stm << ":hidden";
        }
        if (sv.carry_ssu) {
            stm << ":ssu";
        }
        if (sv.carry_t2mi) {
            stm << ":t2mi";
        }
        if (sv.pmt_pid != 0) {
            stm << ":pmtpid=" << sv.pmt_pid;
        }
        if (sv.pcr_pid != 0 && sv.pcr_pid != PID_NULL) {
            stm << ":pcrpid=" << sv.pcr_pid;
        }
        stm << ":pidlist=";
        first = true;
        for (const auto& it_pid : _pids) {
            if (it_pid.second->services.count(sv.service_id) != 0) {
                // This PID belongs to the service
                stm << (first ? "" : ",") << it_pid.first;
                first = false;
            }
        }
        stm << ":provider=" << sv.getProvider() << ":name=" << sv.getName() << std::endl;
    }

    // Print one line per PID
    for (const auto& it : _pids) {
        const PIDContext& pc(*it.second);
        if (pc.ts_pkt_cnt == 0 && pc.optional) {
            continue;
        }
        stm << "pid:pid=" << pc.pid << ":";
        if (pc.is_pmt_pid) {
            stm << "pmt:";
        }
        if (pc.carry_ecm) {
            stm << "ecm:";
        }
        if (pc.carry_emm) {
            stm << "emm:";
        }
        if (pc.cas_id != 0) {
            stm << "cas=" << pc.cas_id << ":";
        }
        for (const auto& it2 : pc.cas_operators) {
            stm << "operator=" << it2 << ":";
        }
        stm << "access=" << (pc.scrambled ? "scrambled" : "clear") << ":";
        if (pc.crypto_period != 0 && _ts_bitrate != 0) {
            stm << "cryptoperiod=" << ((pc.crypto_period * PKT_SIZE_BITS) / _ts_bitrate).toInt() << ":";
        }
        if (pc.same_stream_id) {
            stm << "streamid=" << int(pc.pes_stream_id) << ":";
        }
        if (pc.carry_audio) {
            stm << "audio:";
        }
        if (pc.carry_video) {
            stm << "video:";
        }
        if (!pc.languages.empty()) {
            stm << "language=" << UString::Join(pc.languages, u",") << ":";
        }
        stm << "servcount=" << pc.services.size() << ":";
        if (!pc.referenced) {
            stm << "unreferenced:";
        }
        else if (pc.services.size() == 0) {
            stm << "global:";
        }
        else {
            first = true;
            for (const auto& it1 : pc.services) {
                stm << (first ? "servlist=" : ",") << it1;
                first = false;
            }
            if (!first) {
                stm << ":";
            }
        }
        first = true;
        for (const auto& it1 : pc.ssu_oui) {
            stm << (first ? "ssuoui=" : ",") << it1;
            first = false;
        }
        if (!first) {
            stm << ":";
        }
        if (pc.carry_t2mi) {
            stm << "t2mi:";
            first = true;
            for (const auto& it1 : pc.t2mi_plp_ts) {
                stm << (first ? "plp=" : ",") << int(it1.first);
                first = false;
            }
            if (!first) {
                stm << ":";
            }
        }
        stm << "bitrate=" << pc.bitrate.toInt() << ":"
            << "bitrate204=" << ToBitrate204(pc.bitrate).toInt() << ":"
            << "packets=" << pc.ts_pkt_cnt << ":"
            << "clear=" << (pc.ts_pkt_cnt - pc.ts_sc_cnt - pc.inv_ts_sc_cnt) << ":"
            << "scrambled=" << pc.ts_sc_cnt << ":"
            << "invalidscrambling=" << pc.inv_ts_sc_cnt << ":"
            << "af=" << pc.ts_af_cnt << ":"
            << "pcr=" << pc.pcr_cnt << ":"
            << "pts=" << pc.pts_cnt << ":"
            << "dts=" << pc.dts_cnt << ":"
            << "pcrleap=" << pc.pcr_leap_cnt << ":"
            << "ptsleap=" << pc.pts_leap_cnt << ":"
            << "dtsleap=" << pc.dts_leap_cnt << ":"
            << "discontinuities=" << pc.unexp_discont << ":"
            << "duplicated=" << pc.duplicated << ":";
        if (pc.carry_pes) {
            stm << "pes=" << pc.pl_start_cnt << ":"
                << "invalidpesprefix=" << pc.inv_pes_start << ":";
        }
        else {
            stm << "unitstart=" << pc.unit_start_cnt << ":";
        }
        if (pc.first_pcr != INVALID_PCR) {
            stm << "firstpcr=" << pc.first_pcr << ":lastpcr=" << pc.last_pcr << ":";
        }
        if (pc.first_pts != INVALID_PTS) {
            stm << "firstpts=" << pc.first_pts << ":lastpts=" << pc.last_pts << ":";
        }
        if (pc.first_dts != INVALID_DTS) {
            stm << "firstdts=" << pc.first_dts << ":lastdts=" << pc.last_dts << ":";
        }
        stm << "description=" << pc.fullDescription(true) << std::endl;
    }

    // Print one line per table
    for (const auto& pci : _pids) {
        const PIDContext& pc(*pci.second);
        for (const auto& it : pc.sections) {
            const ETIDContext& etc(*it.second);
            stm << "table:"
                << "pid=" << pc.pid << ":"
                << "tid=" << int(etc.etid.tid()) << ":";
            if (etc.etid.isLongSection()) {
                stm << "tidext=" << etc.etid.tidExt() << ":";
            }
            stm << "tables=" << etc.table_count << ":"
                << "sections=" << etc.section_count << ":"
                << "repetitionpkt=" << etc.repetition_ts << ":"
                << "minrepetitionpkt=" << etc.min_repetition_ts << ":"
                << "maxrepetitionpkt=" << etc.max_repetition_ts << ":";
            if (_ts_bitrate != 0) {
                stm << "repetitionms=" << PacketInterval(_ts_bitrate, etc.repetition_ts) << ":"
                    << "minrepetitionms=" << PacketInterval(_ts_bitrate, etc.min_repetition_ts) << ":"
                    << "maxrepetitionms=" << PacketInterval(_ts_bitrate, etc.max_repetition_ts) << ":";
            }
            if (etc.versions.any()) {
                stm << "firstversion=" << int(etc.first_version) << ":"
                    << "lastversion=" << int(etc.last_version) << ":"
                    << "versions=";
                first = true;
                for (size_t i = 0; i < etc.versions.size(); ++i) {
                    if (etc.versions.test(i)) {
                        stm << (first ? "" : ",") << i;
                        first = false;
                    }
                }
                stm << ":";
            }
            stm << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// This method displays a JSON report.
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportJSON(TSAnalyzerOptions& opt, std::ostream& stm, const UString& title, Report& rep)
{
    // Update the global statistics value if internal data were modified.
    recomputeStatistics();

    // JSON root.
    json::Object root;

    // Add user-supplied title.
    if (!title.empty()) {
        root.add(u"title", title);
    }

    // Add transport stream description
    if (_ts_id.has_value()) {
        root.query(u"ts", true).add(u"id", *_ts_id);
    }
    root.query(u"ts", true).add(u"bytes", PKT_SIZE * _ts_pkt_cnt);
    root.query(u"ts", true).add(u"bitrate", _ts_bitrate.toInt());
    root.query(u"ts", true).add(u"bitrate-204", ToBitrate204(_ts_bitrate).toInt());
    root.query(u"ts", true).add(u"user-bitrate", _ts_user_bitrate.toInt());
    root.query(u"ts", true).add(u"user-bitrate-204", ToBitrate204(_ts_user_bitrate).toInt());
    root.query(u"ts", true).add(u"pcr-bitrate", _ts_pcr_bitrate_188.toInt());
    root.query(u"ts", true).add(u"pcr-bitrate-204", _ts_pcr_bitrate_204.toInt());
    root.query(u"ts", true).add(u"duration", _duration / 1000);
    if (!_country_code.empty()) {
        root.query(u"ts", true).add(u"country", _country_code);
    }

    root.query(u"ts.services", true).add(u"total", _services.size());
    root.query(u"ts.services", true).add(u"clear", _services.size() - _scrambled_services_cnt);
    root.query(u"ts.services", true).add(u"scrambled", _scrambled_services_cnt);

    root.query(u"ts.packets", true).add(u"total", _ts_pkt_cnt);
    root.query(u"ts.packets", true).add(u"invalid-syncs", _invalid_sync);
    root.query(u"ts.packets", true).add(u"transport-errors", _transport_errors);
    root.query(u"ts.packets", true).add(u"suspect-ignored", _suspect_ignored);

    // Add PID's info.
    root.query(u"ts.pids", true).add(u"total", _pid_cnt);
    root.query(u"ts.pids", true).add(u"clear", _pid_cnt - _scrambled_pid_cnt);
    root.query(u"ts.pids", true).add(u"scrambled", _scrambled_pid_cnt);
    root.query(u"ts.pids", true).add(u"pcr", _pcr_pid_cnt);
    root.query(u"ts.pids", true).add(u"unreferenced", _unref_pid_cnt);

    // Global PID's (ie. not attached to a service)
    root.query(u"ts.pids.global", true).add(u"total", _global_pid_cnt);
    root.query(u"ts.pids.global", true).add(u"clear", _global_pid_cnt - _global_scr_pids);
    root.query(u"ts.pids.global", true).add(u"scrambled", _global_scr_pids);
    root.query(u"ts.pids.global", true).add(u"packets", _global_pkt_cnt);
    root.query(u"ts.pids.global", true).add(u"bitrate", _global_bitrate.toInt());
    root.query(u"ts.pids.global", true).add(u"bitrate-204", ToBitrate204(_global_bitrate).toInt());
    root.query(u"ts.pids.global", true).add(u"is-scrambled", json::Bool(_global_scr_pids > 0));
    for (const auto& it : _pids) {
        const PIDContext& pc(*it.second);
        if (pc.referenced && pc.services.size() == 0 && (pc.ts_pkt_cnt != 0 || !pc.optional)) {
            root.query(u"ts.pids.global.pids", true, json::Type::Array).set(pc.pid);
        }
    }

    // Unreferenced PIDs
    root.query(u"ts.pids.unreferenced", true).add(u"total", _unref_pid_cnt);
    root.query(u"ts.pids.unreferenced", true).add(u"clear", _unref_pid_cnt - _unref_scr_pids);
    root.query(u"ts.pids.unreferenced", true).add(u"scrambled", _unref_scr_pids);
    root.query(u"ts.pids.unreferenced", true).add(u"packets", _unref_pkt_cnt);
    root.query(u"ts.pids.unreferenced", true).add(u"bitrate", _unref_bitrate.toInt());
    root.query(u"ts.pids.unreferenced", true).add(u"bitrate-204", ToBitrate204(_unref_bitrate).toInt());
    root.query(u"ts.pids.unreferenced", true).add(u"is-scrambled", json::Bool(_unref_scr_pids > 0));
    for (const auto& it : _pids) {
        const PIDContext& pc (*it.second);
        if (!pc.referenced && (pc.ts_pkt_cnt != 0 || !pc.optional)) {
            root.query(u"ts.pids.unreferenced.pids", true, json::Type::Array).set(pc.pid);
        }
    }

    // Add first and last UTC and local times.
    jsonTime(root, u"time.utc.tdt.first", _first_tdt);
    jsonTime(root, u"time.utc.tdt.last", _last_tdt);
    jsonTime(root, u"time.utc.tdt.first", _first_tdt);
    jsonTime(root, u"time.utc.tdt.last", _last_tdt);
    jsonTime(root, u"time.local.tot.first", _first_tot, _country_code);
    jsonTime(root, u"time.local.tot.last", _last_tot, _country_code);
    if (!opt.deterministic) {
        jsonTime(root, u"time.utc.system.first", _first_utc);
        jsonTime(root, u"time.utc.system.last", _last_utc);
        jsonTime(root, u"time.local.system.first", _first_local);
        jsonTime(root, u"time.local.system.last", _last_local);
    }

    // One node per service
    for (const auto& it : _services) {
        const ServiceContext& sv(*it.second);
        json::Value& jv(root.query(u"services[]", true));
        jv.add(u"id", sv.service_id);
        jv.add(u"provider", sv.getProvider());
        jv.add(u"name", sv.getName());
        jv.add(u"type", sv.service_type);
        jv.add(u"type-name", names::ServiceType(sv.service_type));
        if (_ts_id.has_value()) {
            jv.add(u"tsid", *_ts_id);
        }
        if (sv.orig_netw_id.has_value()) {
            jv.add(u"original-network-id", *sv.orig_netw_id);
        }
        if (sv.lcn.has_value()) {
            jv.add(u"lcn", *sv.lcn);
        }
        jv.add(u"is-scrambled", json::Bool(sv.scrambled_pid_cnt > 0));
        jv.query(u"components", true).add(u"total", sv.pid_cnt);
        jv.query(u"components", true).add(u"clear", sv.pid_cnt - sv.scrambled_pid_cnt);
        jv.query(u"components", true).add(u"scrambled", sv.scrambled_pid_cnt);
        jv.add(u"packets", sv.ts_pkt_cnt);
        jv.add(u"bitrate", sv.bitrate.toInt());
        jv.add(u"bitrate-204", ToBitrate204(sv.bitrate).toInt());
        jv.add(u"hidden", json::Bool(sv.hidden));
        jv.add(u"ssu", json::Bool(sv.carry_ssu));
        jv.add(u"t2mi", json::Bool(sv.carry_t2mi));
        if (sv.pmt_pid != 0) {
            jv.add(u"pmt-pid", sv.pmt_pid);
        }
        if (sv.pcr_pid != 0 && sv.pcr_pid != PID_NULL) {
            jv.add(u"pcr-pid", sv.pcr_pid);
        }
        for (const auto& it_pid : _pids) {
            if (it_pid.second->services.count(sv.service_id) != 0) {
                // This PID belongs to the service
                jv.query(u"pids", true, json::Type::Array).set(it_pid.first);
            }
        }
    }

    // One node per PID
    for (const auto& it : _pids) {
        const PIDContext& pc(*it.second);
        if (pc.ts_pkt_cnt == 0 && pc.optional) {
            continue;
        }
        json::Value& jv(root.query(u"pids[]", true));
        jv.add(u"id", pc.pid);
        jv.add(u"description", pc.fullDescription(true));
        jv.add(u"pmt", json::Bool(pc.is_pmt_pid));
        jv.add(u"audio", json::Bool(pc.carry_audio));
        jv.add(u"video", json::Bool(pc.carry_video));
        jv.add(u"ecm", json::Bool(pc.carry_ecm));
        jv.add(u"emm", json::Bool(pc.carry_emm));
        if (pc.cas_id != 0) {
            jv.add(u"cas", pc.cas_id);
        }
        for (const auto& it2 : pc.cas_operators) {
            jv.query(u"operators", true, json::Type::Array).set(it2);
        }
        jv.add(u"is-scrambled", json::Bool(pc.scrambled));
        if (pc.crypto_period != 0 && _ts_bitrate != 0) {
            jv.add(u"crypto-period", ((pc.crypto_period * PKT_SIZE_BITS) / _ts_bitrate).toInt());
        }
        if (pc.same_stream_id) {
            jv.add(u"pes-stream-id", pc.pes_stream_id);
        }
        if (!pc.languages.empty()) {
            // First language as a string (legacy compatibility).
            jv.add(u"language", pc.languages.front());
            // All languages as an array of string.
            for (const auto& lang : pc.languages) {
                jv.query(u"languages", true, json::Type::Array).set(lang);
            }
        }
        jv.add(u"service-count", pc.services.size());
        jv.add(u"unreferenced", json::Bool(!pc.referenced));
        jv.add(u"global", json::Bool(pc.services.size() == 0));
        for (const auto& it1 : pc.services) {
            jv.query(u"services", true, json::Type::Array).set(it1);
        }
        for (const auto& it1 : pc.ssu_oui) {
            jv.query(u"ssu-oui", true, json::Type::Array).set(it1);
        }
        jv.add(u"t2mi", json::Bool(pc.carry_t2mi));
        for (const auto& it1 : pc.t2mi_plp_ts) {
            jv.query(u"plp", true, json::Type::Array).set(it1.first);
        }
        jv.add(u"bitrate", pc.bitrate.toInt());
        jv.add(u"bitrate-204", ToBitrate204(pc.bitrate).toInt());
        jv.query(u"packets", true).add(u"total", pc.ts_pkt_cnt);
        jv.query(u"packets", true).add(u"clear", pc.ts_pkt_cnt - pc.ts_sc_cnt - pc.inv_ts_sc_cnt);
        jv.query(u"packets", true).add(u"scrambled", pc.ts_sc_cnt);
        jv.query(u"packets", true).add(u"invalid-scrambling", pc.inv_ts_sc_cnt);
        jv.query(u"packets", true).add(u"af", pc.ts_af_cnt);
        jv.query(u"packets", true).add(u"pcr", pc.pcr_cnt);
        jv.query(u"packets", true).add(u"pts", pc.pts_cnt);
        jv.query(u"packets", true).add(u"dts", pc.dts_cnt);
        jv.query(u"packets", true).add(u"pcr-leap", pc.pcr_leap_cnt);
        jv.query(u"packets", true).add(u"pts-leap", pc.pts_leap_cnt);
        jv.query(u"packets", true).add(u"dts-leap", pc.dts_leap_cnt);
        jv.query(u"packets", true).add(u"discontinuities", pc.unexp_discont);
        jv.query(u"packets", true).add(u"duplicated", pc.duplicated);
        if (pc.carry_pes) {
            jv.add(u"pes", pc.pl_start_cnt);
            jv.add(u"invalid-pes-prefix", pc.inv_pes_start);
        }
        else {
            jv.add(u"unit-start", pc.unit_start_cnt);
        }
        if (pc.first_pcr != INVALID_PCR) {
            jv.add(u"first-pcr", pc.first_pcr);
            jv.add(u"last-pcr", pc.last_pcr);
        }
        if (pc.first_pts != INVALID_PTS) {
            jv.add(u"first-pts", pc.first_pts);
            jv.add(u"last-pts", pc.last_pts);
        }
        if (pc.first_dts != INVALID_DTS) {
            jv.add(u"first-dts", pc.first_dts);
            jv.add(u"last-dts", pc.last_dts);
        }
    }

    // One node per table
    for (const auto& pci : _pids) {
        const PIDContext& pc(*pci.second);
        for (const auto& it : pc.sections) {
            const ETIDContext& etc(*it.second);
            json::Value& jv(root.query(u"tables[]", true));
            jv.add(u"pid", pc.pid);
            jv.add(u"tid", etc.etid.tid());
            if (etc.etid.isLongSection()) {
                jv.add(u"tid-ext", etc.etid.tidExt());
            }
            jv.add(u"tables", etc.table_count);
            jv.add(u"sections", etc.section_count);
            jv.add(u"repetition-pkt", etc.repetition_ts);
            jv.add(u"min-repetition-pkt", etc.min_repetition_ts);
            jv.add(u"max-repetition-pkt", etc.max_repetition_ts);
            if (_ts_bitrate != 0) {
                jv.add(u"repetition-ms", PacketInterval(_ts_bitrate, etc.repetition_ts));
                jv.add(u"min-repetition-ms", PacketInterval(_ts_bitrate, etc.min_repetition_ts));
                jv.add(u"max-repetition-ms", PacketInterval(_ts_bitrate, etc.max_repetition_ts));
            }
            if (etc.versions.any()) {
                jv.add(u"first-version", etc.first_version);
                jv.add(u"last-version", etc.last_version);
                for (size_t i = 0; i < etc.versions.size(); ++i) {
                    if (etc.versions.test(i)) {
                        jv.query(u"versions", true, json::Type::Array).set(i);
                    }
                }
            }
        }
    }

    // An output text formatter for JSON output.
    opt.json.report(root, stm, rep);
}


//----------------------------------------------------------------------------
// This static method builds a JSON time.
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::jsonTime(json::Value& parent, const UString& path, const Time& time, const UString& country)
{
    if (time != Time::Epoch) {
        json::Value& tm(parent.query(path, true));
        tm.add(u"date", time.format(Time::DATE));
        tm.add(u"time", time.format(Time::TIME | Time::MILLISECOND));
        tm.add(u"seconds-since-2000", (time - Time(2000, 1, 1, 0, 0, 0)) / MilliSecPerSec);
        if (!country.empty()) {
            tm.add(u"country", country);
        }
    }
}
