//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
#include "tsStringUtils.h"
#include "tsDecimal.h"
#include "tsFormat.h"
#include "tsNames.h"



//----------------------------------------------------------------------------
// Set analysis options. Must be set before feeding the first packet.
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::setAnalysisOptions (const TSAnalyzerOptions& opt)
{
    setMinErrorCountBeforeSuspect (opt.suspect_min_error_count);
    setMaxConsecutiveSuspectCount (opt.suspect_max_consecutive);
}


//----------------------------------------------------------------------------
// General reporting method, using options
//----------------------------------------------------------------------------

std::ostream& ts::TSAnalyzerReport::report (std::ostream& strm, const TSAnalyzerOptions& opt)
{
    // Perform one-line reports

    size_t count = 0;

    if (opt.service_list) {
        // List of service ids
        std::vector<uint16_t> list;
        getServiceIds (list);
        for (size_t i = 0; i < list.size(); ++i) {
            strm << (count++ > 0 ? " " : "") << opt.prefix << int (list[i]);
        }
    }

    if (opt.pid_list) {
        // List of PIDs
        std::vector<PID> list;
        getPIDs (list);
        for (size_t i = 0; i < list.size(); ++i) {
            strm << (count++ > 0 ? " " : "") << opt.prefix << int (list[i]);
        }
    }

    if (opt.global_pid_list) {
        // List of global PIDs
        std::vector<PID> list;
        getGlobalPIDs (list);
        for (size_t i = 0; i < list.size(); ++i) {
            strm << (count++ > 0 ? " " : "") << opt.prefix << int (list[i]);
        }
    }

    if (opt.unreferenced_pid_list) {
        // List of unreferenced PIDs
        std::vector<PID> list;
        getUnreferencedPIDs (list);
        for (size_t i = 0; i < list.size(); ++i) {
            strm << (count++ > 0 ? " " : "") << opt.prefix << int (list[i]);
        }
    }

    if (opt.service_pid_list) {
        // List of PIDs for one service
        std::vector<PID> list;
        getPIDsOfService (list, opt.service_id);
        for (size_t i = 0; i < list.size(); ++i) {
            strm << (count++ > 0 ? " " : "") << opt.prefix << int (list[i]);
        }
    }

    if (opt.pes_pid_list) {
        // List of PIDs carrying PES packets
        std::vector<PID> list;
        getPIDsWithPES (list);
        for (size_t i = 0; i < list.size(); ++i) {
            strm << (count++ > 0 ? " " : "") << opt.prefix << int (list[i]);
        }
    }

    if (count > 0) {
        strm << std::endl;
    }
    if (opt.ts_analysis) {
        reportTS (strm, opt.title);
    }
    if (opt.service_analysis) {
        reportServices (strm, opt.title);
    }
    if (opt.pid_analysis) {
        reportPIDs (strm, opt.title);
    }
    if (opt.table_analysis) {
        reportTables (strm, opt.title);
    }
    if (opt.error_analysis) {
        reportErrors (strm, opt.title);
    }
    if (opt.normalized) {
        reportNormalized (strm, opt.title);
    }

    return strm;
}


//----------------------------------------------------------------------------
//  Displays a report header
//----------------------------------------------------------------------------

namespace {
    void ReportHeader (std::ostream& strm, const std::string& title1, const std::string& title2)
    {
        using namespace ts;
        size_t len1 (title1.length());
        size_t len2 (title2.length());
        strm << std::endl << std::string (79, '=') << std::endl;
        if (len1 + len2 <= 71) {
            strm << "|  " << title1 << std::string (73 - len1 - len2, ' ') << title2 << "  |" << std::endl;
        }
        else {
            strm << "|  " << JustifyLeft (title1, 73) << "  |" << std::endl;
            size_t start (0);
            while (start < len2) {
                strm << "|  " << JustifyRight (title2.substr (start, 73), 73, ' ', true) << "  |" << std::endl;
                start += 73;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Report global transport stream analysis
//----------------------------------------------------------------------------

std::ostream& ts::TSAnalyzerReport::reportTS (std::ostream& strm, const std::string& title)
{
    // Update the global statistics value if internal data were modified.
    recomputeStatistics ();

    ReportHeader (strm, "TRANSPORT STREAM ANALYSIS REPORT", title);

    strm << '|' << std::string (77, '=') << '|' << std::endl <<
        
        "|  Transport Stream Id: " <<
        JustifyRight (_ts_id_valid ? Format (" %d (0x%04X)", (int)_ts_id, (int)_ts_id) : " Unknown", 21, '.') <<
        "  |  Services: " <<
        JustifyRight (" " + Decimal (_services.size()), 16, '.') <<
        "  |" << std::endl <<

        "|  Bytes: " <<
        JustifyRight (" " + Decimal (PKT_SIZE * _ts_pkt_cnt), 35, '.') <<
        "  |  PID's: Total: " <<
        JustifyRight (Format (" %" FMT_SIZE_T "u", _pid_cnt), 12, '.') <<
        "  |" << std::endl <<
        
        "|  TS packets: " <<
        JustifyRight (" " + Decimal (_ts_pkt_cnt), 30, '.') <<
        "  |         Clear: " <<
        JustifyRight (Format (" %" FMT_SIZE_T "u", _pid_cnt - _scrambled_pid_cnt), 12, '.') <<
        "  |" << std::endl <<

        "|     With invalid sync: " <<
        JustifyRight (" " + Decimal (_invalid_sync), 20, '.') <<
        "  |         Scrambled: " <<
        JustifyRight (Format (" %" FMT_SIZE_T "u", _scrambled_pid_cnt), 8, '.') <<
        "  |" << std::endl <<

        "|     With transport error: " <<
        JustifyRight (" " + Decimal (_transport_errors), 17, '.') <<
        "  |         With PCR's: " <<
        JustifyRight (Format (" %" FMT_SIZE_T "u", _pcr_pid_cnt), 7, '.') <<
        "  |" << std::endl <<

        "|     Suspect and ignored: " <<
        JustifyRight (" " + Decimal (_suspect_ignored), 18, '.') <<
        "  |         Unreferenced: " <<
        JustifyRight (Format (" %" FMT_SIZE_T "u", _unref_pid_cnt), 5, '.') <<
        "  |" << std::endl <<

        '|' << std::string (77, '-') << '|' << std::endl <<
        "|  Transport stream bitrate, based on ....... 188 bytes/pkt    204 bytes/pkt  |" << std::endl <<
        "|  User-specified: ";

    if (_ts_user_bitrate == 0)
        strm << "................................... None             None";
    else {
        strm << JustifyRight (" " + Decimal (_ts_user_bitrate) + " b/s ", 41, '.')
             << JustifyRight (Decimal (ToBitrate204 (_ts_user_bitrate)) + " b/s", 16, ' ');
    }
    strm << "  |" << std::endl << "|  Estimated based on PCR's: ";
    if (_ts_pcr_bitrate_188 == 0)
        strm << "...................... Unknown          Unknown";
    else {
        strm << JustifyRight (" " + Decimal (_ts_pcr_bitrate_188) + " b/s ", 31, '.')
             << JustifyRight (Decimal (_ts_pcr_bitrate_204) + " b/s", 16, ' ');
    }
    strm << "  |" << std::endl <<
        '|' << std::string (77, '-') << '|' << std::endl
        << "|  Broadcast time: ";
    if (_duration == 0)
        strm << JustifyRight (" Unknown", 57, '.');
    else {
        strm << JustifyRight (" " + Decimal (_duration / 1000) + " sec (" + 
                              Decimal (_duration / 60000)+ " mn " + 
                              Format ("%d", int ((_duration / 1000) % 60)) + " sec)", 57, '.');
    }
    strm << "  |" << std::endl <<
        "|  First TDT UTC time stamp: " <<
        JustifyRight (_first_tdt == Time::Epoch ? " Unknown" : " " + _first_tdt.format (Time::DATE | Time::TIME), 47, '.') <<
        "  |" << std::endl <<
        "|  Last TDT UTC time stamp: " <<
        JustifyRight (_last_tdt == Time::Epoch ? " Unknown" : " " + _last_tdt.format (Time::DATE | Time::TIME), 48, '.') <<
        "  |" << std::endl <<
        "|  First TOT local time stamp: " <<
        JustifyRight (_first_tot == Time::Epoch ? " Unknown" : " " + _first_tot.format (Time::DATE | Time::TIME), 45, '.') <<
        "  |" << std::endl <<
        "|  Last TOT local time stamp: " <<
        JustifyRight (_last_tot == Time::Epoch ? " Unknown" : " " + _last_tot.format (Time::DATE | Time::TIME), 46, '.') <<
        "  |" << std::endl <<
        "|  TOT country code: " <<
        JustifyRight (_country_code.empty() ? " Unknown" : " " + Printable (_country_code), 55, '.') <<
        "  |" << std::endl;

    // Display list of services

    strm << '|' << std::string (77, '-') << '|' << std::endl <<
        "| Serv.Id  Service Name                              Access          Bitrate  |" << std::endl;

    for (ServiceContextMap::const_iterator it = _services.begin(); it != _services.end(); ++it) {
        const ServiceContext& sv (*it->second);
        strm << Format ("|  0x%04X  ", int (sv.service_id))
             << JustifyLeft (sv.getName() + " ", 45, '.')
             << "  " << (sv.scrambled_pid_cnt > 0 ? 'S' : 'C')
             << JustifyRight (sv.bitrate == 0 ? "Unknown" : Decimal (sv.bitrate) + " b/s", 17)
             << "  |" << std::endl;
    }

    strm << "|" << std::string (77, ' ') << "|" << std::endl <<
        "|  Note 1: C=Clear, S=Scrambled                                               |" << std::endl <<
        "|  Note 2: Unless explicitely specified otherwise, all bitrates are based on  |" << std::endl <<
        "|  188 bytes per packet.                                                      |" << std::endl <<
        std::string (79, '=') << std::endl << std::endl;

    return strm;
}


//----------------------------------------------------------------------------
// Display header of a service PID list 
//----------------------------------------------------------------------------

namespace {
    void ReportServiceHeader (std::ostream& strm,
                              const std::string& usage,
                              bool scrambled,
                              ts::BitRate bitrate,
                              ts::BitRate ts_bitrate)
    {
        using namespace ts;
        strm << '|' << std::string (77, '-') << '|' << std::endl <<
            "|     PID  Usage                                     Access          Bitrate  |" << std::endl <<
            "|   Total  " << JustifyLeft (usage + "  ", 45, '.', true) << "  " << (scrambled ? 'S' : 'C') << ' ';
        if (ts_bitrate == 0)
            strm << "         Unknown";
        else
            strm << JustifyRight (Decimal (bitrate), 12) << " b/s";
        strm << "  |" << std::endl;
    }
}

//----------------------------------------------------------------------------
// Display one line of a service PID list 
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportServicePID (std::ostream& strm, const PIDContext& pc) const
{
    std::string description (pc.fullDescription (true));
    if (!pc.ssu_oui.empty()) {
        bool first = true;
        for (std::set<uint32_t>::const_iterator it = pc.ssu_oui.begin(); it != pc.ssu_oui.end(); ++it) {
            description += first ? " (SSU " : ", ";
            description += names::OUI (*it);
            first = false;
        }
        description += ")";
    }
    strm << Format ("|  0x%04X  ", int (pc.pid))
         << JustifyLeft (description + "  ", 45, '.', true) << "  "
         << (pc.scrambled ? 'S' : 'C') << (pc.services.size() > 1 ? '+' : ' ');
    if (_ts_bitrate == 0) {
        strm << "         Unknown";
    }
    else {
        strm << JustifyRight (Decimal (pc.bitrate), 12) << " b/s";
    }
    strm << "  |" << std::endl;
}


//----------------------------------------------------------------------------
// Report services analysis
//----------------------------------------------------------------------------

std::ostream& ts::TSAnalyzerReport::reportServices (std::ostream& strm, const std::string& title)
{
    // Update the global statistics value if internal data were modified.
    recomputeStatistics ();

    ReportHeader (strm, "SERVICES ANALYSIS REPORT", title);

    // Display global pids

    strm << '|' << std::string (77, '=') << '|' << std::endl <<
        "|  Global PID's                                                               |" << std::endl <<
        JustifyLeft ("|  TS packets: " + Decimal (_global_pkt_cnt) + 
                     Format (", PID's: %" FMT_SIZE_T "u (clear: %" FMT_SIZE_T "u, scrambled: %" FMT_SIZE_T "u)",
                             _global_pid_cnt, _global_pid_cnt - _global_scr_pids, _global_scr_pids),
                     78) <<
        '|' << std::endl;

    ReportServiceHeader (strm, "Global PID's", _global_scr_pids > 0, _global_bitrate, _ts_bitrate);

    for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {
        const PIDContext& pc (*it->second);
        if (pc.referenced && pc.services.empty() && (pc.ts_pkt_cnt != 0 || !pc.optional))
            reportServicePID (strm, pc);
    }

    // Display unreferenced pids

    if (_unref_pid_cnt > 0) {
        strm << '|' << std::string (77, '=') << '|' << std::endl <<
            "|  Unreferenced PID's                                                         |" << std::endl <<
            JustifyLeft ("|  TS packets: " + Decimal (_unref_pkt_cnt) + 
                         Format (", PID's: %" FMT_SIZE_T "u (clear: %" FMT_SIZE_T "u, scrambled: %" FMT_SIZE_T "u)",
                                 _unref_pid_cnt, _unref_pid_cnt - _unref_scr_pids, _unref_scr_pids),
                         78) <<
            '|' << std::endl;

        ReportServiceHeader (strm, "Unreferenced PID's", _unref_scr_pids > 0, _unref_bitrate, _ts_bitrate);

        for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {
            const PIDContext& pc (*it->second);
            if (!pc.referenced && (pc.ts_pkt_cnt != 0 || !pc.optional))
                reportServicePID (strm, pc);
        }
    }

    // Display list of services

    for (ServiceContextMap::const_iterator it = _services.begin(); it != _services.end(); ++it) {

        const ServiceContext& sv (*it->second);

        strm << '|' << std::string (77, '=') << '|' << std::endl
             << JustifyLeft (Format ("|  Service: %d (0x%04X), TS: %d (0x%04X), Original Netw: %d (0x%04X)",
                                     int (sv.service_id), int (sv.service_id),
                                     int (_ts_id), int (_ts_id),
                                     int (sv.orig_netw_id), int (sv.orig_netw_id)),
                             78)
             << '|' << std::endl
             << JustifyLeft ("|  Service name: " + sv.getName() + ", provider: " + sv.getProvider(), 78, ' ', true)
             << '|' << std::endl
             << JustifyLeft (Format ("|  Service type: %d (0x%02X), ", int (sv.service_type), int (sv.service_type)) +
                             names::ServiceType (sv.service_type),
                             78, ' ', true)
             << '|' << std::endl
             << JustifyLeft ("|  TS packets: " + Decimal (sv.ts_pkt_cnt) +
                             Format (", PID's: %" FMT_SIZE_T "u (clear: %" FMT_SIZE_T "u, scrambled: %" FMT_SIZE_T "u)",
                                     sv.pid_cnt, sv.pid_cnt - sv.scrambled_pid_cnt, sv.scrambled_pid_cnt),
                             78)
             << '|' << std::endl;

        std::string pmt_pid;
        std::string pcr_pid;

        if (sv.pmt_pid == 0 || sv.pmt_pid == PID_NULL) {
            pmt_pid = "Unknown in PAT";
        }
        else {
            pmt_pid = Format ("%d (0x%04X)", int (sv.pmt_pid), int (sv.pmt_pid));
        }

        if (sv.pcr_pid == 0 || sv.pcr_pid == PID_NULL) {
            pcr_pid = "None";
        }
        else {
            pcr_pid = Format ("%d (0x%04X)", int (sv.pcr_pid), int (sv.pcr_pid));
        }

        strm << JustifyLeft ("|  PMT PID: " + pmt_pid + ", PCR PID: " + pcr_pid, 78) << '|' << std::endl;

        // Display all PID's of this service

        ReportServiceHeader (strm, names::ServiceType (sv.service_type),
                             sv.scrambled_pid_cnt > 0, sv.bitrate, _ts_bitrate);

        for (PIDContextMap::const_iterator pid_it = _pids.begin(); pid_it != _pids.end(); ++pid_it) {
            const PIDContext& pc (*pid_it->second);
            if (pc.services.find (sv.service_id) != pc.services.end()) {
                reportServicePID (strm, pc);
            }
        }

        strm << "|          (C=Clear, S=Scrambled, +=Shared)                                   |" << std::endl;
    }

    strm << std::string (79, '=') << std::endl << std::endl;
    return strm;
}


//----------------------------------------------------------------------------
// Print list of services a PID belongs to
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportServicesForPID (std::ostream& strm, const PIDContext& pc) const
{
    for (ServiceIdSet::const_iterator it = pc.services.begin(); it != pc.services.end(); ++it) {
        uint16_t serv_id (*it);
        ServiceContextMap::const_iterator serv_it (_services.find (serv_id));
        const std::string serv_name (serv_it == _services.end() ? "" : serv_it->second->getName());
        strm << JustifyLeft (Format ("|  Service: %d (0x%04X) ", int (serv_id), int (serv_id)) + serv_name, 78, ' ', true)
             << '|' << std::endl;
    }
}


//----------------------------------------------------------------------------
// Report PID's analysis.
//----------------------------------------------------------------------------

std::ostream& ts::TSAnalyzerReport::reportPIDs (std::ostream& strm, const std::string& title)
{
    // Update the global statistics value if internal data were modified.
    recomputeStatistics ();

    ReportHeader (strm, "PIDS ANALYSIS REPORT", title);

    for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {
        const PIDContext& pc (*it->second);
        if (pc.ts_pkt_cnt == 0) {
            continue;
        }

        // Header lines
        strm << '|' << std::string (77, '=') << '|' << std::endl
             << JustifyLeft (Format ("|  PID: %d (0x%04X) ", int (pc.pid), int (pc.pid)), 22)
             << JustifyRight (pc.fullDescription (false), 54, ' ', true) << "  |" << std::endl;

        // Type of PES data, if available
        if (pc.same_stream_id) {
            strm << JustifyLeft (Format ("|  PES stream id: 0x%02X (", int (pc.pes_stream_id)) +
                                 names::StreamId (pc.pes_stream_id) + ")", 78, ' ', true)
                 << '|' << std::endl;
        }

        // Audio/video attributes
        for (StringVector::const_iterator it1 = pc.attributes.begin(); it1 != pc.attributes.end(); ++it1) {
            if (!it1->empty()) {
                strm << "|  " << JustifyLeft (*it1, 74, ' ', true) << " |" << std::endl;
            }
        }

        // List of services to which the PID belongs to
        reportServicesForPID (strm, pc);

        // List of System Software Update OUI's on this PID
        for (std::set<uint32_t>::const_iterator it1 = pc.ssu_oui.begin(); it1 != pc.ssu_oui.end(); ++it1) {
            strm << JustifyLeft (Format ("|  SSU OUI: 0x%06X (", int (*it1)) + names::OUI (*it1) + ")", 78, ' ', true)
                 << '|' << std::endl;
        }
        strm << '|' << std::string (77, '-') << '|' << std::endl;

        // 3-columns output. Layout: total width = 79
        // margin=3 col1=24 space=2 col2=24 space=2 col3=21 margin=3

        // Line 1:
        strm << "|  "; // Column 1: (24 chars)
        if (pc.services.size() == 1)
            strm << "Referenced PID          ";
        else if (pc.services.size() > 1)
            strm << "Shared PID              ";
        else if (pc.referenced)
            strm << "Global PID              ";
        else
            strm << "Unreferenced PID        ";
        strm << "  "; // Column 2: (24 chars)
        strm << "Transport:              ";
        strm << "  "; // Column 3: (21 chars)
        strm << "Discontinuities:     ";
        strm << "  |" << std::endl;

        // Line 2:
        strm << "|  "; // Column 1: (24 chars)
        if (_ts_bitrate == 0)
            strm << "Bitrate: Unknown        ";
        else
            strm << Justify ("Bitrate: ", " " + Decimal (pc.bitrate) + " b/s", 24, '.');
        strm << "  "; // Column 2: (24 chars)
        strm << Justify ("Packets: ", " " + Decimal (pc.ts_pkt_cnt), 24, '.');
        strm << "  "; // Column 3: (21 chars)
        strm << Justify ("Expected: ", " " + Decimal (pc.exp_discont), 21, '.');
        strm << "  |" << std::endl;

        // Line 3:
        strm << "|  "; // Column 1: (24 chars)
        strm << Format ("Access: %-16s", pc.scrambled ? "Scrambled" : "Clear");
        strm << "  "; // Column 2: (24 chars)
        strm << Justify ("Adapt.F.: ", " " + Decimal (pc.ts_af_cnt), 24, '.');
        strm << "  "; // Column 3: (21 chars)
        strm << Justify ("Unexpect: ", " " + Decimal (pc.unexp_discont), 21, '.');
        strm << "  |" << std::endl;

        // Line 4:
        strm << "|  "; // Column 1: (24 chars)
        if (!pc.scrambled)
            strm << "                        ";
        else if (pc.crypto_period == 0)
            strm << "Crypto-Period: Unknown  ";
        else
            strm << "Crypto-Period:          ";
        strm << "  "; // Column 2: (24 chars)
        strm << Justify ("Duplicated: ", " " + Decimal (pc.duplicated), 24, '.');
        strm << "  "; // Column 3: (21 chars)
        strm << Format ("%-21s", pc.carry_pes ? "PES:" : "Sections:");
        strm << "  |" << std::endl;

        // Line 5:
        strm << "|  "; // Column 1: (24 chars)
        if (!pc.scrambled || pc.crypto_period == 0)
            strm << "                        ";
        else if (_ts_bitrate == 0)
            strm << JustifyRight (" " + Decimal (pc.crypto_period) + " TS packets", 24, '.');
        else
            strm << JustifyRight (Format (" %d seconds", int ((pc.crypto_period * PKT_SIZE * 8) / _ts_bitrate)), 24, '.');
        strm << "  "; // Column 2: (24 chars)
        strm << Justify ("PCR: ", " " + Decimal (pc.pcr_cnt), 24, '.');
        strm << "  "; // Column 3: (21 chars)
        if (pc.carry_pes)
            strm << Justify ("Packets: ", " " + Decimal (pc.pl_start_cnt), 21, '.');
        else
            strm << Justify ("Unit start: ", " " + Decimal (pc.unit_start_cnt), 21, '.');
        strm << "  |" << std::endl;

        // Line 6:
        strm << "|  "; // Column 1: (24 chars)
        strm << Format ("%-24s", pc.scrambled ? "Inv. scrambling ctrl:" : "");
        strm << "  "; // Column 2: (24 chars)
        if (pc.ts_pcr_bitrate > 0)
            strm << Justify ("TSrate: ", " " + Decimal (pc.ts_pcr_bitrate) + " b/s", 24, '.');
        else
            strm << "                        ";
        strm << "  "; // Column 3: (21 chars)
        strm << Format ("%-21s", pc.carry_pes ? "Inv. PES start code:" : "");
        strm << "  |" << std::endl;

        // Line 7:
        if (pc.scrambled || pc.ts_pcr_bitrate > 0 || pc.carry_pes) {
            strm << "|  "; // Column 1: (24 chars)
            if (pc.scrambled)
                strm << JustifyRight (" " + Decimal (pc.inv_ts_sc_cnt), 24, '.');
            else
                strm << "                        ";
            strm << "  "; // Column 2: (24 chars)
            strm << "                        ";
            strm << "  "; // Column 3: (21 chars)
            if (pc.carry_pes)
                strm << JustifyRight (" " + Decimal (pc.inv_pes_start), 21, '.');
            else
                strm << "                     ";
            strm << "  |" << std::endl;
        }
    }

    strm << std::string (79, '=') << std::endl << std::endl;
    return strm;
}


//----------------------------------------------------------------------------
// Report tables analysis
//----------------------------------------------------------------------------

std::ostream& ts::TSAnalyzerReport::reportTables (std::ostream& strm, const std::string& title)
{
    // Update the global statistics value if internal data were modified.
    recomputeStatistics ();

    ReportHeader (strm, "TABLES & SECTIONS ANALYSIS REPORT", title);

    // Loop on all PID's
    for (PIDContextMap::const_iterator pci = _pids.begin(); pci != _pids.end(); ++pci) {
        const PIDContext& pc (*pci->second);

        // Skip PID's without sections
        if (pc.sections.empty()) {
            continue;
        }

        // Header line: PID
        strm << '|' << std::string (77, '=') << '|' << std::endl
             << JustifyLeft (Format ("|  PID: %d (0x%04X) ", int (pc.pid), int (pc.pid)), 22)
             << JustifyRight (pc.fullDescription (false), 54, ' ', true) << "  |" << std::endl;

        // Header lines: list of services to which the PID belongs to
        reportServicesForPID (strm, pc);

        // Loop on all tables on this PID
        for (ETIDContextMap::const_iterator it = pc.sections.begin(); it != pc.sections.end(); ++it) {
            const ETIDContext& etc (*it->second);
            const TID tid = etc.etid.tid();

            // Header line: TID
            const std::string tid_name (names::TID (tid, CASFamilyOf (pc.cas_id)));
            const std::string tid_ext (etc.etid.isShortSection() ? "" :
                                       Format (", TID ext: %d (0x%04X)", int (etc.etid.tidExt()), int (etc.etid.tidExt())));
            strm << '|' << std::string (77, '-') << '|' << std::endl
                 << JustifyLeft (Format ("|  TID: %d (0x%04X), %s%s", int (tid), int (tid), tid_name.c_str(), tid_ext.c_str()), 77)
                 << " |" << std::endl;

            // Repetition rates are displayed in ms if the TS bitrate is known, in packets otherwise.
            const char* unit;
            uint64_t rep, min_rep, max_rep;
            if (_ts_bitrate != 0) {
                unit = " ms";
                rep = PacketInterval (_ts_bitrate, etc.repetition_ts);
                min_rep = PacketInterval (_ts_bitrate, etc.min_repetition_ts);
                max_rep = PacketInterval (_ts_bitrate, etc.max_repetition_ts);
            }
            else {
                unit = " pkt";
                rep = etc.repetition_ts;
                min_rep = etc.min_repetition_ts;
                max_rep = etc.max_repetition_ts;
            }

            // Version description
            const size_t version_count = etc.versions.count();
            std::string version_list;
            bool first = true;
            for (size_t i = 0; i < etc.versions.size(); ++i) {
                if (etc.versions.test (i)) {
                    if (!first) {
                        version_list.append (", ");
                    }
                    version_list.append (Format ("%" FMT_SIZE_T "d", i));
                    first = false;
                }
            }

            // 3-columns output. Layout: total width = 79
            // margin=7 col1=25 space=2 col2=23 space=2 col3=17 margin=3

            // Line 1:
            strm << "|      "; // Column 1: (25 chars)
            strm << Justify ("Repetition: ", " " + Decimal (rep) + unit, 25, '.');
            strm << "  ";      // Column 2: (23 chars)
            strm << Justify ("Section cnt: ", " " + Decimal (etc.section_count), 23, '.');
            strm << "  ";      // Column 3: (17 chars)
            if (version_count > 1) {
                strm << Justify ("First version:", Decimal (etc.first_version), 17, ' ');
            }
            else {
                strm << std::string (17, ' ');
            }
            strm << "  |" << std::endl;

            // Line 2:
            strm << "|      "; // Column 1: (25 chars)
            strm << Justify ("Min repet.: ", " " + Decimal (min_rep) + unit, 25, '.');
            if (etc.etid.isShortSection()) {
                strm << std::string (2 + 23 + 2 + 17, ' ');
            }
            else {
                strm << "  ";      // Column 2: (23 chars)
                strm << Justify ("Table cnt: ", " " + Decimal (etc.table_count), 23, '.');
                strm << "  ";      // Column 3: (17 chars)
                if (version_count > 1) {
                    strm << Justify ("Last version:", Decimal (etc.last_version), 17, ' ');
                }
                else {
                    strm << std::string (17, ' ');
                }
            }
            strm << "  |" << std::endl;

            // Line 3:
            strm << "|      "; // Column 1: (25 chars)
            strm << Justify ("Max repet.: ", " " + Decimal (max_rep) + unit, 25, '.');
            if (etc.etid.isShortSection()) {
                strm << std::string (2 + 23 + 2 + 17, ' ');
            }
            else if (version_count == 1) {
                strm << "  ";      // Column 2: (23 chars)
                strm << Justify ("Version: ", " " + version_list, 23, '.');
                strm << std::string (2 + 17, ' ');
            }
            else if (version_list.length() <= 12) {
                strm << "  ";      // Column 2: (23 chars)
                strm << Justify ("Versions: ", " " + version_list, 23, '.');
                strm << std::string (2 + 17, ' ');
            }
            else {
                strm << "  ";
                strm << JustifyLeft ("Versions: " + version_list, 23 + 2 + 17, ' ', true);
            }

            strm << "  |" << std::endl;
        }
    }

    strm << std::string (79, '=') << std::endl << std::endl;
    return strm;
}


//----------------------------------------------------------------------------
// This methods displays an error report
//----------------------------------------------------------------------------

std::ostream& ts::TSAnalyzerReport::reportErrors (std::ostream& strm, const std::string& title)
{
    int error_count = 0;

    // Update the global statistics value if internal data were modified.
    recomputeStatistics ();

    // Header
    strm << "TITLE: ERROR ANALYSIS REPORT" << std::endl;
    if (title != "") {
        strm << "TITLE: " << title << std::endl;
    }
    if (_ts_id_valid) {
        strm << "INFO: Transport Stream Identifier: " << _ts_id << Format (" (0x%04X)", int (_ts_id)) << std::endl;
    }

    // Report global errors

    if (_invalid_sync > 0) {
        error_count++;
        strm << Format ("TS:%d:0x%04X: TS packets with invalid sync byte: %d", int (_ts_id), int (_ts_id), int (_invalid_sync)) << std::endl;
    }
    if (_transport_errors > 0) {
        error_count++;
        strm << Format ("TS:%d:0x%04X: TS packets with transport error indicator: %d", int (_ts_id), int (_ts_id), int (_transport_errors)) << std::endl;
    }
    if (_suspect_ignored > 0) {
        error_count++;
        strm << Format ("TS:%d:0x%04X: suspect TS packets, ignored: %d", int (_ts_id), int (_ts_id), int (_suspect_ignored)) << std::endl;
    }
    if (_unref_pid_cnt > 0) {
        error_count++;
        strm << Format ("TS:%d:0x%04X: Unreferenced PID's: %d", int (_ts_id), int (_ts_id), int (_unref_pid_cnt)) << std::endl;
    }

    // Report missing standard DVB tables

    if (!_tid_present[TID_PAT]) {
        error_count++;
        strm << Format ("TS:%d:0x%04X: No PAT", int (_ts_id), int (_ts_id)) << std::endl;
    }
    if (_scrambled_pid_cnt > 0 && !_tid_present[TID_CAT]) {
        error_count++;
        strm << Format ("TS:%d:0x%04X: No CAT (%d scrambled PID's)", int (_ts_id), int (_ts_id), int (_scrambled_pid_cnt)) << std::endl;
    }
    if (!_tid_present[TID_SDT_ACT]) {
        error_count++;
        strm << Format ("TS:%d:0x%04X: No SDT Actual", int (_ts_id), int (_ts_id)) << std::endl;
    }
    if (!_tid_present[TID_BAT]) {
        error_count++;
        strm << Format ("TS:%d:0x%04X: No BAT", int (_ts_id), int (_ts_id)) << std::endl;
    }
    if (!_tid_present[TID_TDT]) {
        error_count++;
        strm << Format ("TS:%d:0x%04X: No TDT", int (_ts_id), int (_ts_id)) << std::endl;
    }
    if (!_tid_present[TID_TOT]) {
        error_count++;
        strm << Format ("TS:%d:0x%04X: No TOT", int (_ts_id), int (_ts_id)) << std::endl;
    }

    // Report error per PID

    for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {
        const PIDContext& pc (*it->second);
        if (pc.exp_discont > 0) {
            error_count++;
            strm << Format ("PID:%d:0x%04X: ", int (pc.pid), int (pc.pid))
                 << "Discontinuities (expected): " << pc.exp_discont << std::endl;
        }
        if (pc.unexp_discont > 0) {
            error_count++;
            strm << Format ("PID:%d:0x%04X: ", int (pc.pid), int (pc.pid))
                 << "Discontinuities (unexpected): " << pc.unexp_discont << std::endl;
        }
        if (pc.duplicated > 0) {
            error_count++;
            strm << Format ("PID:%d:0x%04X: ", int (pc.pid), int (pc.pid))
                 << "Duplicated TS packets: " << pc.duplicated << std::endl;
        }
        if (pc.inv_ts_sc_cnt > 0) {
            error_count++;
            strm << Format ("PID:%d:0x%04X: ", int (pc.pid), int (pc.pid))
                 << "Invalid scrambling control values: " << pc.inv_ts_sc_cnt << std::endl;
        }
        if (pc.carry_pes && pc.inv_pes_start > 0) {
            error_count++;
            strm << Format ("PID:%d:0x%04X: ", int (pc.pid), int (pc.pid))
                 << "Invalid PES header start codes: " << pc.inv_pes_start << std::endl;
        }
        if (pc.is_pmt_pid && pc.pmt_cnt == 0) {
            assert (!pc.services.empty());
            int service_id (*(pc.services.begin()));
            error_count++;
            strm << Format ("PID:%d:0x%04X: ", int (pc.pid), int (pc.pid))
                 << "No PMT (PMT PID of service " << Format ("%d, 0x%04X)", service_id, service_id) << std::endl;
        }
        if (pc.is_pcr_pid && pc.pcr_cnt == 0) {
            error_count++;
            strm << Format ("PID:%d:0x%04X: ", int (pc.pid), int (pc.pid)) << "No PCR (PCR PID of service";
            if (pc.services.size() > 1) {
                strm << "s";
            }
            for (ServiceIdSet::const_iterator i = pc.services.begin(); i != pc.services.end(); ++i) {
                if (i != pc.services.begin()) {
                    strm << ",";
                }
                strm << Format (" %d (0x%04X)", int (*i),int (*i));
            }
            strm << ")" << std::endl;
        }
    }

    // Summary

    strm << "SUMMARY: Error count: " << error_count << std::endl;
    return strm;
}


//----------------------------------------------------------------------------
// This static method displays a normalized time value.
//----------------------------------------------------------------------------

void ts::TSAnalyzerReport::reportNormalizedTime (std::ostream& strm, const Time& time, const char* type, const std::string& country)
{
    if (time != Time::Epoch) {
        Time::Fields f (time);
        strm << type << ":"
             << Format ("date=%02d/%02d/%04d:", f.day, f.month, f.year)
             << Format ("time=%02dh%02dm%02ds:", f.hour, f.minute, f.second)
             << "secondsince2000=" << ((time - Time (2000, 1, 1, 0, 0, 0)) / MilliSecPerSec) << ":";
        if (!country.empty()) {
            strm << "country=" << Printable (country) << ":";
        }
        strm << std::endl;
    }
}


//----------------------------------------------------------------------------
// This method displays a normalized report.
//----------------------------------------------------------------------------

std::ostream& ts::TSAnalyzerReport::reportNormalized (std::ostream& strm, const std::string& title)
{
    // Update the global statistics value if internal data were modified.

    recomputeStatistics ();

    // Print one line with user-supplied title

    strm << "title:" << title << std::endl;

    // Print one line with transport stream description

    strm << "ts:";
    if (_ts_id_valid) {
        strm << "id=" << _ts_id << ":";
    }
    strm << "services=" << _services.size() << ":"
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
         << "bitrate204=" << ToBitrate204 (_ts_bitrate) << ":"
         << "userbitrate=" << _ts_user_bitrate << ":"
         << "userbitrate204=" << ToBitrate204 (_ts_user_bitrate) << ":"
         << "pcrbitrate=" << _ts_pcr_bitrate_188 << ":"
         << "pcrbitrate204=" << _ts_pcr_bitrate_204 << ":"
         << "duration=" << (_duration / 1000) << ":";
    if (!_country_code.empty()) {
        strm << "country=" << Printable (_country_code) << ":";
    }
    strm << std::endl;

    // Print lines for first and last UTC and local time

    reportNormalizedTime (strm, _first_tdt,   "time:utc:tdt:first");
    reportNormalizedTime (strm, _last_tdt,    "time:utc:tdt:last");
    reportNormalizedTime (strm, _first_tot,   "time:local:tot:first", _country_code);
    reportNormalizedTime (strm, _last_tot,    "time:local:tot:last", _country_code);
    reportNormalizedTime (strm, _first_utc,   "time:utc:system:first");
    reportNormalizedTime (strm, _last_utc,    "time:utc:system:last");
    reportNormalizedTime (strm, _first_local, "time:local:system:first");
    reportNormalizedTime (strm, _last_local,  "time:local:system:last");

    // Print one line for global PIDs

    strm << "global:"
         << "pids=" << _global_pid_cnt << ":"
         << "clearpids=" << (_global_pid_cnt - _global_scr_pids) << ":"
         << "scrambledpids=" << _global_scr_pids << ":"
         << "packets=" << _global_pkt_cnt << ":"
         << "bitrate=" << _global_bitrate << ":"
         << "bitrate204=" << ToBitrate204 (_global_bitrate) << ":"
         << "access=" << (_global_scr_pids > 0 ? "scrambled" : "clear") << ":"
         << "pidlist=";
    bool first = true;
    for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {
        const PIDContext& pc (*it->second);
        if (pc.referenced && pc.services.size() == 0 && (pc.ts_pkt_cnt != 0 || !pc.optional)) {
            strm << (first ? "" : ",") << pc.pid;
            first = false;
        }
    }
    strm << ":" << std::endl;

    // Print one line for unreferenced PIDs

    strm << "unreferenced:"
         << "pids=" << _unref_pid_cnt << ":"
         << "clearpids=" << (_unref_pid_cnt - _unref_scr_pids) << ":"
         << "scrambledpids=" << _unref_scr_pids << ":"
         << "packets=" << _unref_pkt_cnt << ":"
         << "bitrate=" << _unref_bitrate << ":"
         << "bitrate204=" << ToBitrate204 (_unref_bitrate) << ":"
         << "access=" << (_unref_scr_pids > 0 ? "scrambled" : "clear") << ":"
         << "pidlist=";
    first = true;
    for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {
        const PIDContext& pc (*it->second);
        if (!pc.referenced && (pc.ts_pkt_cnt != 0 || !pc.optional)) {
            strm << (first ? "" : ",") << pc.pid;
            first = false;
        }
    }
    strm << ":" << std::endl;

    // Print one line per service

    for (ServiceContextMap::const_iterator it = _services.begin(); it != _services.end(); ++it) {
        const ServiceContext& sv (*it->second);
        strm << "service:"
             << "id=" << sv.service_id << ":"
             << "tsid=" << _ts_id << ":"
             << "orignetwid=" << sv.orig_netw_id << ":"
             << "access=" << (sv.scrambled_pid_cnt > 0 ? "scrambled" : "clear") << ":"
             << "pids=" << sv.pid_cnt << ":"
             << "clearpids=" << (sv.pid_cnt - sv.scrambled_pid_cnt) << ":"
             << "scrambledpids=" << sv.scrambled_pid_cnt << ":"
             << "packets=" << sv.ts_pkt_cnt << ":"
             << "bitrate=" << sv.bitrate << ":"
             << "bitrate204=" << ToBitrate204 (sv.bitrate) << ":"
             << "servtype=" << int (sv.service_type) << ":";
        if (sv.carry_ssu) {
            strm << "ssu:";
        }
        if (sv.pmt_pid != 0) {
            strm << "pmtpid=" << sv.pmt_pid << ":";
        }
        if (sv.pcr_pid != 0 && sv.pcr_pid != PID_NULL) {
            strm << "pcrpid=" << sv.pcr_pid << ":";
        }
        strm << "pidlist=";
        first = true;
        for (PIDContextMap::const_iterator it_pid = _pids.begin(); it_pid != _pids.end(); ++it_pid) {
            if (it_pid->second->services.count (sv.service_id) != 0) {
                // This PID belongs to the service
                strm << (first ? "" : ",") << it_pid->first;
                first = false;
            }
        }
        strm << ":"
             << "provider=" << sv.getProvider() << ":"
             << "name=" << sv.getName()
             << std::endl;
    }

    // Print one line per PID

    for (PIDContextMap::const_iterator it = _pids.begin(); it != _pids.end(); ++it) {
        const PIDContext& pc (*it->second);
        if (pc.ts_pkt_cnt == 0 && pc.optional) {
            continue;
        }
        strm << "pid:pid=" << pc.pid << ":";
        if (pc.is_pmt_pid) {
            strm << "pmt:";
        }
        if (pc.carry_ecm) {
            strm << "ecm:";
        }
        if (pc.carry_emm) {
            strm << "emm:";
        }
        if (pc.cas_id != 0) {
            strm << "cas=" << pc.cas_id << ":";
        }
        if (pc.cas_operator != 0) {
            strm << "operator=" << pc.cas_operator << ":";
        }
        strm << "access=" << (pc.scrambled ? "scrambled" : "clear") << ":";
        if (pc.crypto_period != 0 && _ts_bitrate != 0) {
            strm << "cryptoperiod=" << ((pc.crypto_period * PKT_SIZE * 8) / _ts_bitrate) << ":";
        }
        if (pc.same_stream_id) {
            strm << "streamid=" << int (pc.pes_stream_id) << ":";
        }
        if (pc.carry_audio) {
            strm << "audio:";
        }
        if (pc.carry_video) {
            strm << "video:";
        }
        if (pc.language != "") {
            strm << "language=" << Printable (pc.language) << ":";
        }
        strm << "servcount=" << pc.services.size() << ":";
        if (!pc.referenced) {
            strm << "unreferenced:";
        }
        else if (pc.services.size() == 0) {
            strm << "global:";
        }
        else {
            first = true;
            for (ServiceIdSet::const_iterator it1 = pc.services.begin(); it1 != pc.services.end(); ++it1) {
                strm << (first ? "servlist=" : ",") << *it1;
                first = false;
            }
            if (!first) {
                strm << ":";
            }
        }
        first = true;
        for (std::set<uint32_t>::const_iterator it1 = pc.ssu_oui.begin(); it1 != pc.ssu_oui.end(); ++it1) {
            strm << (first ? "ssuoui=" : ",") << *it1;
            first = false;
        }
        if (!first) {
            strm << ":";
        }
        strm << "bitrate=" << pc.bitrate << ":"
             << "bitrate204=" << ToBitrate204 (pc.bitrate) << ":"
             << "packets=" << pc.ts_pkt_cnt << ":"
             << "clear=" << (pc.ts_pkt_cnt - pc.ts_sc_cnt - pc.inv_ts_sc_cnt) << ":"
             << "scrambled=" << pc.ts_sc_cnt << ":"
             << "invalidscrambling=" << pc.inv_ts_sc_cnt << ":"
             << "af=" << pc.ts_af_cnt << ":"
             << "pcr=" << pc.pcr_cnt << ":"
             << "discontinuities=" << pc.unexp_discont << ":"
             << "duplicated=" << pc.duplicated << ":";
        if (pc.carry_pes) {
            strm << "pes=" << pc.pl_start_cnt << ":"
                 << "invalidpesprefix=" << pc.inv_pes_start << ":";
        }
        else {
            strm << "unitstart=" << pc.unit_start_cnt << ":";
        }
        strm << "description=" << pc.fullDescription (true) << std::endl;
    }

    // Print one line per table

    for (PIDContextMap::const_iterator pci = _pids.begin(); pci != _pids.end(); ++pci) {
        const PIDContext& pc (*pci->second);
        for (ETIDContextMap::const_iterator it = pc.sections.begin(); it != pc.sections.end(); ++it) {
            const ETIDContext& etc (*it->second);
            strm << "table:"
                 << "pid=" << pc.pid << ":"
                 << "tid=" << int (etc.etid.tid()) << ":";
            if (etc.etid.isLongSection()) {
                strm << "tidext=" << etc.etid.tidExt() << ":";
            }
            strm << "tables=" << etc.table_count << ":"
                 << "sections=" << etc.section_count << ":"
                 << "repetitionpkt=" << etc.repetition_ts << ":"
                 << "minrepetitionpkt=" << etc.min_repetition_ts << ":"
                 << "maxrepetitionpkt=" << etc.max_repetition_ts << ":";
            if (_ts_bitrate != 0) {
                strm << "repetitionms=" << PacketInterval (_ts_bitrate, etc.repetition_ts) << ":"
                     << "minrepetitionms=" << PacketInterval (_ts_bitrate, etc.min_repetition_ts) << ":"
                     << "maxrepetitionms=" << PacketInterval (_ts_bitrate, etc.max_repetition_ts) << ":";
            }
            if (etc.versions.any()) {
                strm << "firstversion=" << int (etc.first_version) << ":"
                     << "lastversion=" << int (etc.last_version) << ":"
                     << "versions=";
                first = true;
                for (size_t i = 0; i < etc.versions.size(); ++i) {
                    if (etc.versions.test (i)) {
                        strm << (first ? "" : ",") << i;
                        first = false;
                    }
                }
                strm << ":";
            }
            strm << std::endl;
        }
    }

    return strm;
}
