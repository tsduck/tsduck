//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  A subclass of TSAnalyzer with reporting capabilities
//
//----------------------------------------------------------------------------

#include "tsTSAnalyzerReport.h"
#include "tsNames.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Set analysis options. Must be set before feeding the first packet.
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::setAnalysisOptions(const TSAnalyzerOptions& opt)
{
    setMinErrorCountBeforeSuspect(opt.suspect_min_error_count);
    setMaxConsecutiveSuspectCount(opt.suspect_max_consecutive);
    setDefaultCharacterSet(opt.default_charset);
}


//----------------------------------------------------------------------------
// General reporting method, using options
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::report(std::ostream& stm, const TSAnalyzerOptions& opt)
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

    // If user has requested decimal pids then make output wider as it's hard to get everything fitted otherwise
    if (opt.service_analysis_decimal_pids)
    {
        grid.setLineWidth(94, 2);
    }
    else
    {
        grid.setLineWidth(79, 2);
    }


    if (opt.ts_analysis) {
        reportTS(grid, opt.title);
    }
    if (opt.service_analysis) {
        reportServices(grid, opt.service_analysis_decimal_pids, opt.title);
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
        reportNormalized(stm, opt.title);
    }
}


//----------------------------------------------------------------------------
// Report global transport stream analysis
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportTS(Grid& grid, const UString& title)
{
    // Update the global statistics value if internal data were modified.
    recomputeStatistics();

    grid.openTable();
    grid.putLine(u"TRANSPORT STREAM ANALYSIS REPORT", title);
    grid.section();

    grid.setLayout({grid.bothTruncateLeft(42, u'.'), grid.border(), grid.bothTruncateLeft(26, u'.')});
    grid.putLayout({{u"Transport Stream Id:", _ts_id_valid ? UString::Format(u"%d (0x%X)", {_ts_id, _ts_id}) : u"Unknown"},
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

    grid.setLayout({grid.bothTruncateLeft(56, u'.'), grid.right(15)});
    grid.putLayout({{u"Transport stream bitrate, based on", u"188 bytes/pkt"},
                    {u"204 bytes/pkt"}});
    grid.putLayout({{u"User-specified:", _ts_user_bitrate == 0 ? u"None" : UString::Format(u"%'d b/s", {_ts_user_bitrate})},
                    {_ts_user_bitrate == 0 ? u"None" : UString::Format(u"%'d b/s", {ToBitrate204(_ts_user_bitrate)})}});
    grid.putLayout({{u"Estimated based on PCR's:", _ts_pcr_bitrate_188 == 0 ? u"Unknown" : UString::Format(u"%'d b/s", {_ts_pcr_bitrate_188})},
                    { _ts_pcr_bitrate_188 == 0 ? u"Unknown" : UString::Format(u"%'d b/s", {_ts_pcr_bitrate_204})}});
    grid.subSection();

    grid.setLayout({grid.bothTruncateLeft(73, u'.')});
    grid.putLayout({{u"Broadcast time:", _duration == 0 ? u"Unknown" : UString::Format(u"%d sec (%d mn %d sec)", {_duration / 1000, _duration / 60000, (_duration / 1000) % 60})}});
    grid.putLayout({{u"First TDT UTC time stamp:", _first_tdt == Time::Epoch ? u"Unknown" : _first_tdt.format(Time::DATE | Time::TIME)}});
    grid.putLayout({{u"Last TDT UTC time stamp:", _last_tdt == Time::Epoch ? u"Unknown" : _last_tdt.format(Time::DATE | Time::TIME)}});
    grid.putLayout({{u"First TOT local time stamp:", _first_tot == Time::Epoch ? u"Unknown" : _first_tot.format(Time::DATE | Time::TIME)}});
    grid.putLayout({{u"Last TOT local time stamp:", _last_tot == Time::Epoch ? u"Unknown" : _last_tot.format(Time::DATE | Time::TIME)}});
    grid.putLayout({{u"TOT country code:", _country_code.empty() ? u" Unknown" : _country_code}});
    grid.subSection();

    // Display list of services

    grid.setLayout({grid.right(6), grid.bothTruncateLeft(48), grid.right(15)});
    grid.putLayout({{u"Srv Id"}, {u"Service Name", u"Access"}, {u"Bitrate"}});
    grid.setLayout({grid.right(6), grid.bothTruncateLeft(48, u'.'), grid.right(15)});

    for (ServiceContextMap::const_iterator it = _services.begin(); it != _services.end(); ++it) {
        const ServiceContext& sv(*it->second);
        grid.putLayout({{UString::Format(u"0x%X", {sv.service_id})},
                        {sv.getName(), sv.scrambled_pid_cnt > 0 ? u"S" : u"C"},
                        {sv.bitrate == 0 ? u"Unknown" : UString::Format(u"%'d b/s", {sv.bitrate})}});
    }

    grid.putLine();
    grid.putLine(u"Note 1: C=Clear, S=Scrambled");
    grid.putMultiLine(u"Note 2: Unless explicitly specified otherwise, all bitrates are based on 188 bytes per packet.");

    grid.closeTable();
}


//----------------------------------------------------------------------------
// Display header of a service PID list
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportServiceHeader(Grid& grid, const UString& usage, bool scrambled, BitRate bitrate, BitRate ts_bitrate, const bool decimalPids) const{
    grid.subSection();
    grid.setLayout({decimalPids?grid.both(14):grid.right(6), grid.bothTruncateLeft(decimalPids?56:49), grid.right(14)});
    grid.putLayout({{u"PID", u""}, {u""}, {u"Usage", u"Access "}, {u"Bitrate"}});
    grid.setLayout({decimalPids?grid.both(14):grid.right(6), grid.bothTruncateLeft(decimalPids?56:49, u'.'), grid.right(14)});
    grid.putLayout({{u"Total", u""}, {usage, scrambled ? u"S " : u"C "}, {ts_bitrate == 0 ? u"Unknown" : UString::Format(u"%'d b/s", {bitrate})}});
}


//----------------------------------------------------------------------------
// Display one line of a service PID list
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportServicePID(Grid& grid, const PIDContext& pc, const bool decimalPids) const
{
    const UString access{pc.scrambled ? u'S' : u'C', pc.services.size() > 1 ? u'+' : u' '};
    UString description(pc.fullDescription(true));
    if (!pc.ssu_oui.empty()) {
        bool first = true;
        for (std::set<uint32_t>::const_iterator it = pc.ssu_oui.begin(); it != pc.ssu_oui.end(); ++it) {
            description += first ? u" (SSU " : u", ";
            description += names::OUI(*it);
            first = false;
        }
        description += u")";
    }

    if (decimalPids)
    {
        grid.putLayout({{UString::Format(u"0x%X", {pc.pid}), UString::Format(u"(%d)", {pc.pid})}, {description, access}, {_ts_bitrate == 0 ? u"Unknown" : UString::Format(u"%'d b/s", {pc.bitrate})}});
    }
    else
    {
        grid.putLayout({{UString::Format(u"0x%X", {pc.pid})}, {description, access}, {_ts_bitrate == 0 ? u"Unknown" : UString::Format(u"%'d b/s", {pc.bitrate})}});
    }
}


//----------------------------------------------------------------------------
// Report services analysis
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportServices(Grid& grid, const bool decimalPids, const UString& title)
{
    // Update the global statistics value if internal data were modified.
    recomputeStatistics();

    grid.openTable();
    grid.putLine(u"SERVICES ANALYSIS REPORT", title);

    // Display global pids

    grid.section();
    grid.putLine(u"Global PID's");
    grid.putLine(UString::Format(u"TS packets: %'d, PID's: %d (clear: %d, scrambled: %d)", {_global_pkt_cnt, _global_pid_cnt, _global_pid_cnt - _global_scr_pids, _global_scr_pids}));
    reportServiceHeader(grid, u"Global PID's", _global_scr_pids > 0, _global_bitrate, _ts_bitrate, decimalPids);

    for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {
        const PIDContext& pc(*it->second);
        if (pc.referenced && pc.services.empty() && (pc.ts_pkt_cnt != 0 || !pc.optional)) {
            reportServicePID(grid, pc, decimalPids);
        }
    }

    // Display unreferenced pids

    if (_unref_pid_cnt > 0) {
        grid.section();
        grid.putLine(u"Unreferenced PID's");
        grid.putLine(UString::Format(u"TS packets: %'d, PID's: %d (clear: %d, scrambled: %d)", {_unref_pkt_cnt, _unref_pid_cnt, _unref_pid_cnt - _unref_scr_pids, _unref_scr_pids}));
        reportServiceHeader(grid, u"Unreferenced PID's", _unref_scr_pids > 0, _unref_bitrate, _ts_bitrate, decimalPids);

        for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {
            const PIDContext& pc(*it->second);
            if (!pc.referenced && (pc.ts_pkt_cnt != 0 || !pc.optional)) {
                reportServicePID(grid, pc, decimalPids);
            }
        }
    }

    // Display list of services

    for (ServiceContextMap::const_iterator it = _services.begin(); it != _services.end(); ++it) {

        const ServiceContext& sv(*it->second);
        grid.section();
        grid.putLine(UString::Format(u"Service: 0x%X (%d), TS: 0x%X (%d), Original Netw: 0x%X (%d)", {sv.service_id, sv.service_id, _ts_id, _ts_id, sv.orig_netw_id, sv.orig_netw_id}));
        grid.putLine(UString::Format(u"Service name: %s, provider: %s", {sv.getName(), sv.getProvider()}));
        grid.putLine(u"Service type: " + names::ServiceType(sv.service_type, names::FIRST));
        grid.putLine(UString::Format(u"TS packets: %'d, PID's: %d (clear: %d, scrambled: %d)", {sv.ts_pkt_cnt, sv.pid_cnt, sv.pid_cnt - sv.scrambled_pid_cnt, sv.scrambled_pid_cnt}));
        grid.putLine(u"PMT PID: " +
                     (sv.pmt_pid == 0 || sv.pmt_pid == PID_NULL ? u"Unknown in PAT" : UString::Format(u"0x%X (%d)", {sv.pmt_pid, sv.pmt_pid})) +
                     u", PCR PID: " +
                     (sv.pcr_pid == 0 || sv.pcr_pid == PID_NULL ? u"None" : UString::Format(u"0x%X (%d)", {sv.pcr_pid, sv.pcr_pid})));

        // Display all PID's of this service
        reportServiceHeader(grid, names::ServiceType(sv.service_type), sv.scrambled_pid_cnt > 0, sv.bitrate, _ts_bitrate, decimalPids);
        for (PIDContextMap::const_iterator pid_it = _pids.begin(); pid_it != _pids.end(); ++pid_it) {
            const PIDContext& pc(*pid_it->second);
            if (pc.services.find(sv.service_id) != pc.services.end()) {
                reportServicePID(grid, pc, decimalPids);
            }
        }

        grid.setLayout({grid.both(decimalPids?14:6), grid.bothTruncateLeft(decimalPids?56:49), grid.right(14)});

        grid.putLayout({{u""}, {u"(C=Clear, S=Scrambled, +=Shared)"}, {u""}});
    }

    grid.closeTable();
}


//----------------------------------------------------------------------------
// Print list of services a PID belongs to.
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportServicesForPID(Grid& grid, const PIDContext& pc) const
{
    for (ServiceIdSet::const_iterator it = pc.services.begin(); it != pc.services.end(); ++it) {
        const uint16_t serv_id = *it;
        ServiceContextMap::const_iterator serv_it(_services.find(serv_id));
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
    for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {

        // Get PID description, ignore if no packet was found.
        // A PID can be declared, in a PMT for instance, but has no traffic on it.
        const PIDContext& pc(*it->second);
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
            crypto_period = UString::Format(u"%d sec", {(pc.crypto_period * PKT_SIZE * 8) / _ts_bitrate});
        }

        // Header lines
        grid.section();
        grid.putLine(UString::Format(u"PID: 0x%X (%d)", {pc.pid, pc.pid}), pc.fullDescription(false), false);

        // Type of PES data, if available
        if (pc.same_stream_id) {
            grid.putLine(u"PES stream id: " + names::StreamId(pc.pes_stream_id, names::FIRST));
        }

        // Audio/video attributes
        for (UStringVector::const_iterator it1 = pc.attributes.begin(); it1 != pc.attributes.end(); ++it1) {
            if (!it1->empty()) {
                grid.putLine(*it1);
            }
        }

        // List of services to which the PID belongs to
        reportServicesForPID(grid, pc);

        // List of System Software Update OUI's on this PID
        for (std::set<uint32_t>::const_iterator it1 = pc.ssu_oui.begin(); it1 != pc.ssu_oui.end(); ++it1) {
            grid.putLine(u"SSU OUI: " + names::OUI(*it1, names::FIRST));
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
                        {u"PCR:", UString::Decimal(pc.pcr_cnt)},
                        {pc.carry_pes ? u"Packets:" : u"Unit start:", UString::Decimal(pc.carry_pes ? pc.pl_start_cnt : pc.unit_start_cnt)}});

        if (pc.ts_pcr_bitrate > 0 || pc.carry_pes) {
            grid.putLayout({{u""},
                            {pc.ts_pcr_bitrate > 0 ? u"TSrate:" : u"", pc.ts_pcr_bitrate > 0 ? UString::Format(u"%'d b/s", {pc.ts_pcr_bitrate}) : u""},
                            {pc.carry_pes ? u"Inv.Start:" : u"", pc.carry_pes ? UString::Decimal(pc.inv_pes_start) : u""}});
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
    for (PIDContextMap::const_iterator pci = _pids.begin(); pci != _pids.end(); ++pci) {

        // Get PID description, ignore if PID without sections
        const PIDContext& pc(*pci->second);
        if (pc.sections.empty()) {
            continue;
        }

        // Header line: PID
        grid.section();
        grid.putLine(UString::Format(u"PID: 0x%X (%d)", {pc.pid, pc.pid}), pc.fullDescription(false), false);

        // Header lines: list of services to which the PID belongs to
        reportServicesForPID(grid, pc);

        // Loop on all tables on this PID
        for (ETIDContextMap::const_iterator it = pc.sections.begin(); it != pc.sections.end(); ++it) {
            const ETIDContext& etc(*it->second);
            const TID tid = etc.etid.tid();
            const bool isShort = etc.etid.isShortSection();

            // Repetition rates are displayed in ms if the TS bitrate is known, in packets otherwise.
            const UChar* unit = 0;
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
            const UChar* version_title = 0;
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
            grid.putLine(names::TID(tid, CASFamilyOf(pc.cas_id), names::BOTH_FIRST) +
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

    // Update the global statistics value if internal data were modified.
    recomputeStatistics();

    // Header
    stm << "TITLE: ERROR ANALYSIS REPORT" << std::endl;
    if (!title.empty()) {
        stm << "TITLE: " << title << std::endl;
    }
    if (_ts_id_valid) {
        stm << UString::Format(u"INFO: Transport Stream Identifier: %d (0x%X)", {_ts_id, _ts_id}) << std::endl;
    }

    // Report global errors

    if (_invalid_sync > 0) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%X: TS packets with invalid sync byte: %d", {_ts_id, _ts_id, _invalid_sync}) << std::endl;
    }
    if (_transport_errors > 0) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%X: TS packets with transport error indicator: %d", {_ts_id, _ts_id, _transport_errors}) << std::endl;
    }
    if (_suspect_ignored > 0) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%X: suspect TS packets, ignored: %d", {_ts_id, _ts_id, _suspect_ignored}) << std::endl;
    }
    if (_unref_pid_cnt > 0) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%X: Unreferenced PID's: %d", {_ts_id, _ts_id, _unref_pid_cnt}) << std::endl;
    }

    // Report missing standard DVB tables

    if (!_tid_present[TID_PAT]) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%X: No PAT", {_ts_id, _ts_id}) << std::endl;
    }
    if (_scrambled_pid_cnt > 0 && !_tid_present[TID_CAT]) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%X: No CAT (%d scrambled PID's)", {_ts_id, _ts_id, _scrambled_pid_cnt}) << std::endl;
    }
    if (!_tid_present[TID_SDT_ACT]) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%X: No SDT Actual", {_ts_id, _ts_id}) << std::endl;
    }
    if (!_tid_present[TID_BAT]) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%X: No BAT", {_ts_id, _ts_id}) << std::endl;
    }
    if (!_tid_present[TID_TDT]) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%X: No TDT", {_ts_id, _ts_id}) << std::endl;
    }
    if (!_tid_present[TID_TOT]) {
        error_count++;
        stm << UString::Format(u"TS:%d:0x%X: No TOT", {_ts_id, _ts_id}) << std::endl;
    }

    // Report error per PID

    for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {
        const PIDContext& pc(*it->second);
        if (pc.exp_discont > 0) {
            error_count++;
            stm << UString::Format(u"PID:%d:0x%X: Discontinuities (expected): %d", {pc.pid, pc.pid, pc.exp_discont}) << std::endl;
        }
        if (pc.unexp_discont > 0) {
            error_count++;
            stm << UString::Format(u"PID:%d:0x%X: Discontinuities (unexpected): %d", {pc.pid, pc.pid, pc.unexp_discont}) << std::endl;
        }
        if (pc.duplicated > 0) {
            error_count++;
            stm << UString::Format(u"PID:%d:0x%X: Duplicated TS packets: %d", {pc.pid, pc.pid, pc.duplicated}) << std::endl;
        }
        if (pc.inv_ts_sc_cnt > 0) {
            error_count++;
            stm << UString::Format(u"PID:%d:0x%X: Invalid scrambling control values: %d", {pc.pid, pc.pid, pc.inv_ts_sc_cnt}) << std::endl;
        }
        if (pc.carry_pes && pc.inv_pes_start > 0) {
            error_count++;
            stm << UString::Format(u"PID:%d:0x%X: Invalid PES header start codes: %d", {pc.pid, pc.pid, pc.inv_pes_start}) << std::endl;
        }
        if (pc.is_pmt_pid && pc.pmt_cnt == 0) {
            assert(!pc.services.empty());
            int service_id(*(pc.services.begin()));
            error_count++;
            stm << UString::Format(u"PID:%d:0x%X: No PMT (PMT PID of service %d, 0x%X)", {pc.pid, pc.pid, service_id, service_id}) << std::endl;
        }
        if (pc.is_pcr_pid && pc.pcr_cnt == 0) {
            error_count++;
            stm << UString::Format(u"PID:%d:0x%X: No PCR, PCR PID of service%s", {pc.pid, pc.pid, pc.services.size() > 1 ? u"s" : u""});
            for (ServiceIdSet::const_iterator i = pc.services.begin(); i != pc.services.end(); ++i) {
                if (i != pc.services.begin()) {
                    stm << ",";
                }
                stm << UString::Format(u" %d (0x%X)", {*i, *i});
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

void ts::TSAnalyzerReport::reportNormalized(std::ostream& stm, const UString& title)
{
    // Update the global statistics value if internal data were modified.

    recomputeStatistics();

    // Print one line with user-supplied title

    stm << "title:" << title << std::endl;

    // Print one line with transport stream description

    stm << "ts:";
    if (_ts_id_valid) {
        stm << "id=" << _ts_id << ":";
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
        << "bitrate=" << _ts_bitrate << ":"
        << "bitrate204=" << ToBitrate204(_ts_bitrate) << ":"
        << "userbitrate=" << _ts_user_bitrate << ":"
        << "userbitrate204=" << ToBitrate204(_ts_user_bitrate) << ":"
        << "pcrbitrate=" << _ts_pcr_bitrate_188 << ":"
        << "pcrbitrate204=" << _ts_pcr_bitrate_204 << ":"
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
    reportNormalizedTime(stm, _first_utc, "time:utc:system:first");
    reportNormalizedTime(stm, _last_utc, "time:utc:system:last");
    reportNormalizedTime(stm, _first_local, "time:local:system:first");
    reportNormalizedTime(stm, _last_local, "time:local:system:last");

    // Print one line for global PIDs

    stm << "global:"
        << "pids=" << _global_pid_cnt << ":"
        << "clearpids=" << (_global_pid_cnt - _global_scr_pids) << ":"
        << "scrambledpids=" << _global_scr_pids << ":"
        << "packets=" << _global_pkt_cnt << ":"
        << "bitrate=" << _global_bitrate << ":"
        << "bitrate204=" << ToBitrate204(_global_bitrate) << ":"
        << "access=" << (_global_scr_pids > 0 ? "scrambled" : "clear") << ":"
        << "pidlist=";
    bool first = true;
    for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {
        const PIDContext& pc(*it->second);
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
        << "bitrate=" << _unref_bitrate << ":"
        << "bitrate204=" << ToBitrate204(_unref_bitrate) << ":"
        << "access=" << (_unref_scr_pids > 0 ? "scrambled" : "clear") << ":"
        << "pidlist=";
    first = true;
    for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {
        const PIDContext& pc (*it->second);
        if (!pc.referenced && (pc.ts_pkt_cnt != 0 || !pc.optional)) {
            stm << (first ? "" : ",") << pc.pid;
            first = false;
        }
    }
    stm << ":" << std::endl;

    // Print one line per service

    for (ServiceContextMap::const_iterator it = _services.begin(); it != _services.end(); ++it) {
        const ServiceContext& sv(*it->second);
        stm << "service:"
            << "id=" << sv.service_id << ":"
            << "tsid=" << _ts_id << ":"
            << "orignetwid=" << sv.orig_netw_id << ":"
            << "access=" << (sv.scrambled_pid_cnt > 0 ? "scrambled" : "clear") << ":"
            << "pids=" << sv.pid_cnt << ":"
            << "clearpids=" << (sv.pid_cnt - sv.scrambled_pid_cnt) << ":"
            << "scrambledpids=" << sv.scrambled_pid_cnt << ":"
            << "packets=" << sv.ts_pkt_cnt << ":"
            << "bitrate=" << sv.bitrate << ":"
            << "bitrate204=" << ToBitrate204(sv.bitrate) << ":"
            << "servtype=" << int(sv.service_type) << ":";
        if (sv.carry_ssu) {
            stm << "ssu:";
        }
        if (sv.carry_t2mi) {
            stm << "t2mi:";
        }
        if (sv.pmt_pid != 0) {
            stm << "pmtpid=" << sv.pmt_pid << ":";
        }
        if (sv.pcr_pid != 0 && sv.pcr_pid != PID_NULL) {
            stm << "pcrpid=" << sv.pcr_pid << ":";
        }
        stm << "pidlist=";
        first = true;
        for (PIDContextMap::const_iterator it_pid = _pids.begin(); it_pid != _pids.end(); ++it_pid) {
            if (it_pid->second->services.count(sv.service_id) != 0) {
                // This PID belongs to the service
                stm << (first ? "" : ",") << it_pid->first;
                first = false;
            }
        }
        stm << ":"
            << "provider=" << sv.getProvider() << ":"
            << "name=" << sv.getName()
            << std::endl;
    }

    // Print one line per PID

    for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {
        const PIDContext& pc(*it->second);
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
        for (std::set<uint32_t>::const_iterator it2 = pc.cas_operators.begin(); it2 != pc.cas_operators.end(); ++it2) {
            stm << "operator=" << (*it2) << ":";
        }
        stm << "access=" << (pc.scrambled ? "scrambled" : "clear") << ":";
        if (pc.crypto_period != 0 && _ts_bitrate != 0) {
            stm << "cryptoperiod=" << ((pc.crypto_period * PKT_SIZE * 8) / _ts_bitrate) << ":";
        }
        if (pc.same_stream_id) {
            stm << "streamid=" << int (pc.pes_stream_id) << ":";
        }
        if (pc.carry_audio) {
            stm << "audio:";
        }
        if (pc.carry_video) {
            stm << "video:";
        }
        if (!pc.language.empty()) {
            stm << "language=" << pc.language << ":";
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
            for (ServiceIdSet::const_iterator it1 = pc.services.begin(); it1 != pc.services.end(); ++it1) {
                stm << (first ? "servlist=" : ",") << *it1;
                first = false;
            }
            if (!first) {
                stm << ":";
            }
        }
        first = true;
        for (std::set<uint32_t>::const_iterator it1 = pc.ssu_oui.begin(); it1 != pc.ssu_oui.end(); ++it1) {
            stm << (first ? "ssuoui=" : ",") << *it1;
            first = false;
        }
        if (!first) {
            stm << ":";
        }
        if (pc.carry_t2mi) {
            stm << "t2mi:";
            first = true;
            for (std::map<uint8_t, uint64_t>::const_iterator it1 = pc.t2mi_plp_ts.begin(); it1 != pc.t2mi_plp_ts.end(); ++it1) {
                stm << (first ? "plp=" : ",") << int(it1->first);
                first = false;
            }
            if (!first) {
                stm << ":";
            }
        }
        stm << "bitrate=" << pc.bitrate << ":"
            << "bitrate204=" << ToBitrate204(pc.bitrate) << ":"
            << "packets=" << pc.ts_pkt_cnt << ":"
            << "clear=" << (pc.ts_pkt_cnt - pc.ts_sc_cnt - pc.inv_ts_sc_cnt) << ":"
            << "scrambled=" << pc.ts_sc_cnt << ":"
            << "invalidscrambling=" << pc.inv_ts_sc_cnt << ":"
            << "af=" << pc.ts_af_cnt << ":"
            << "pcr=" << pc.pcr_cnt << ":"
            << "discontinuities=" << pc.unexp_discont << ":"
            << "duplicated=" << pc.duplicated << ":";
        if (pc.carry_pes) {
            stm << "pes=" << pc.pl_start_cnt << ":"
                << "invalidpesprefix=" << pc.inv_pes_start << ":";
        }
        else {
            stm << "unitstart=" << pc.unit_start_cnt << ":";
        }
        stm << "description=" << pc.fullDescription(true) << std::endl;
    }

    // Print one line per table

    for (PIDContextMap::const_iterator pci = _pids.begin(); pci != _pids.end(); ++pci) {
        const PIDContext& pc(*pci->second);
        for (ETIDContextMap::const_iterator it = pc.sections.begin(); it != pc.sections.end(); ++it) {
            const ETIDContext& etc(*it->second);
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
