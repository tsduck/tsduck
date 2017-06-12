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
//  Transport stream processor shared library:
//  Count TS packets.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsDecimal.h"
#include "tsMemoryUtils.h"



//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class CountPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        CountPlugin (TSP*);
        virtual bool start();
        virtual bool stop();
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

    private:
        std::ofstream _outfile;            // User-specified output file
        bool          _negate;             // Negate filter (exclude selected packets)
        PIDSet        _pids;               // PID values to filter
        bool          _brief_report;       // Display biref report, values but not comments
        bool          _report_all;         // Report packet index and PID of each packet
        bool          _report_summary;     // Report summary
        bool          _report_total;       // Report total of all PIDs
        PacketCounter _current_pkt;        // Current TS packet number
        PacketCounter _counters [PID_MAX]; // Packet counter per PID

        // Report a line
        void report (const char*, ...) TS_PRINTF_FORMAT (2, 3);

        // Inaccessible operations
        CountPlugin() = delete;
        CountPlugin(const CountPlugin&) = delete;
        CountPlugin& operator=(const CountPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::CountPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::CountPlugin::CountPlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, "Count TS packets per PID.", "[options]"),
    _outfile(),
    _negate(false),
    _pids(),
    _brief_report(false),
    _report_all(false),
    _report_summary(false),
    _report_total(false),
    _current_pkt(0),
    _counters()
{
    option ("all",         'a');
    option ("brief",       'b');
    option ("negate",      'n');
    option ("output-file", 'o', STRING);
    option ("pid",         'p', PIDVAL, 0, UNLIMITED_COUNT);
    option ("summary",     's');
    option ("total",       't');

    setHelp ("Options:\n"
             "\n"
             "  -a\n"
             "  --all\n"
             "      Report packet index and PID for all packets from the selected PID's.\n"
             "      By default, only a final summary is reported.\n"
             "\n"
             "  -b\n"
             "  --brief\n"
             "      Brief display. Report only the numerical values, not comment on their\n"
             "      usage.\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -n\n"
             "  --negate\n"
             "      Negate the filter: specified PID's are excluded.\n"
             "\n"
             "  -o filename\n"
             "  --output-file filename\n"
             "      Specify the output file for reporting packet counters. By default, report\n"
             "      on standard error using the tsp logging mechanism.\n"
             "\n"
             "  -p value\n"
             "  --pid value\n"
             "      PID filter: select packets with this PID value. Several -p or --pid\n"
             "      options may be specified. By default, if --pid is not specified, all\n"
             "      PID's are selected.\n"
             "\n"
             "  -s\n"
             "  --summary\n"
             "      Display a final summary of packet counts per PID. This is the default,\n"
             "      unless --all or --total is specified, in which case the final summary is\n"
             "      reported only if --summary is specified.\n"
             "\n"
             "  -t\n"
             "  --total\n"
             "      Display the total packet counts in all PID's.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::CountPlugin::start()
{
    _report_all = present ("all");
    _report_total = present ("total");
    _report_summary = (!_report_all && !_report_total) || present ("summary");
    _brief_report = present ("brief");
    _negate = present ("negate");
    getPIDSet (_pids, "pid");

    // By default, all PIDs are selected
    if (!present ("pid")) {
        _pids.set();
    }

    // Create output file
    if (present ("output-file")) {
        const std::string name (value ("output-file"));
        tsp->verbose ("creating " + name);
        _outfile.open (name.c_str(), std::ios::out);
        if (!_outfile) {
            tsp->error ("cannot create " + name);
            return false;
        }
    }

    // Reset state
    _current_pkt = 0;
    TS_ZERO (_counters);

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::CountPlugin::stop()
{
    // Display final report
    if (_report_summary) {
        for (size_t pid = 0; pid < PID_MAX; pid++) {
            if (_counters[pid] > 0) {
                if (_brief_report) {
                    report ("%d %" FMT_INT64 "u", int (pid), _counters[pid]);
                }
                else {
                    const std::string count (Decimal (_counters[pid]));
                    report ("PID %4d (0x%04X): %10s packets", int (pid), int (pid), count.c_str());
                }
            }
        }
    }
    if (_report_total) {
        PacketCounter total = 0;
        for (size_t pid = 0; pid < PID_MAX; pid++) {
            total += _counters[pid];
        }
        if (_brief_report) {
            report ("%" FMT_INT64 "u", total);
        }
        else {
            const std::string count1 (Decimal (total));
            const std::string count2 (Decimal (_current_pkt));
            report ("total: counted %s packets out of %s", count1.c_str(), count2.c_str());
        }
    }

    // Close output file
    if (_outfile.is_open()) {
        _outfile.close();
    }

    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::CountPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Check if the packet must be counted
    const PID pid = pkt.getPID();
    bool ok = _pids[pid];
    if (_negate) {
        ok = !ok;
    }

    // Report packets
    if (ok) {
        if (_report_all) {
            if (_brief_report) {
                report ("%" FMT_INT64 "u %d", _current_pkt, int (pid));
            }
            else {
                const std::string curpkt (Decimal (_current_pkt));
                report ("Packet: %10s, PID: %4d (0x%04X)", curpkt.c_str(), int (pid), int (pid));
            }
        }
        _counters[pid]++;
    }

    // Count TS packets
    _current_pkt++;
    return TSP_OK;
}


//----------------------------------------------------------------------------
// Report a history line
//----------------------------------------------------------------------------

void ts::CountPlugin::report (const char* format, ...)
{
    std::string line;
    TS_FORMAT_STRING (line, format);

    if (_outfile.is_open()) {
        _outfile << line << std::endl;
    }
    else {
        tsp->info (line);
    }
}
