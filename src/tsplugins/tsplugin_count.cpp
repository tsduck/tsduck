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
#include "tsTime.h"
#include "tsDecimal.h"
#include "tsMemoryUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class CountPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        CountPlugin(TSP*);
        virtual bool start();
        virtual bool stop();
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket(TSPacket&, bool&, bool&);

    private:
        // This structure is used at each --interval.
        struct IntervalReport
        {
            Time          start;            // Interval start time in UTC.
            PacketCounter counted_packets;  // Total counted packets.
            PacketCounter total_packets;    // Total TS packets.

            // Constructor.
            IntervalReport() : start(), counted_packets(0), total_packets(0) {}
        };

        std::ofstream  _outfile;            // User-specified output file
        bool           _negate;             // Negate filter (exclude selected packets)
        PIDSet         _pids;               // PID values to filter
        bool           _brief_report;       // Display biref report, values but not comments
        bool           _report_all;         // Report packet index and PID of each packet
        bool           _report_summary;     // Report summary
        bool           _report_total;       // Report total of all PIDs
        PacketCounter  _current_pkt;        // Current TS packet number
        PacketCounter  _report_interval;    // If non-zero, report time-stamp at this packet interval
        IntervalReport _last_report;        // Last report content
        PacketCounter  _counters[PID_MAX];  // Packet counter per PID

        // Report a line
        void report(const char*, ...) TS_PRINTF_FORMAT(2, 3);

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

ts::CountPlugin::CountPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Count TS packets per PID.", u"[options]"),
    _outfile(),
    _negate(false),
    _pids(),
    _brief_report(false),
    _report_all(false),
    _report_summary(false),
    _report_total(false),
    _current_pkt(0),
    _report_interval(0),
    _last_report(),
    _counters()
{
    option(u"all",         'a');
    option(u"brief",       'b');
    option(u"interval",    'i', UINT32);
    option(u"negate",      'n');
    option(u"output-file", 'o', STRING);
    option(u"pid",         'p', PIDVAL, 0, UNLIMITED_COUNT);
    option(u"summary",     's');
    option(u"total",       't');

    setHelp(u"Options:\n"
            u"\n"
            u"  -a\n"
            u"  --all\n"
            u"      Report packet index and PID for all packets from the selected PID's.\n"
            u"      By default, only a final summary is reported.\n"
            u"\n"
            u"  -b\n"
            u"  --brief\n"
            u"      Brief display. Report only the numerical values, not comment on their\n"
            u"      usage.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -i value\n"
            u"  --interval value\n"
            u"      Report a time-stamp and global packet count at regular intervals. The\n"
            u"      specified value is a number of packets.\n"
            u"\n"
            u"  -n\n"
            u"  --negate\n"
            u"      Negate the filter: specified PID's are excluded.\n"
            u"\n"
            u"  -o filename\n"
            u"  --output-file filename\n"
            u"      Specify the output file for reporting packet counters. By default, report\n"
            u"      on standard error using the tsp logging mechanism.\n"
            u"\n"
            u"  -p value\n"
            u"  --pid value\n"
            u"      PID filter: select packets with this PID value. Several -p or --pid\n"
            u"      options may be specified. By default, if --pid is not specified, all\n"
            u"      PID's are selected.\n"
            u"\n"
            u"  -s\n"
            u"  --summary\n"
            u"      Display a final summary of packet counts per PID. This is the default,\n"
            u"      unless --all or --total is specified, in which case the final summary is\n"
            u"      reported only if --summary is specified.\n"
            u"\n"
            u"  -t\n"
            u"  --total\n"
            u"      Display the total packet counts in all PID's.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::CountPlugin::start()
{
    _report_all = present(u"all");
    _report_total = present(u"total");
    _report_summary = (!_report_all && !_report_total) || present(u"summary");
    _brief_report = present(u"brief");
    _negate = present(u"negate");
    getIntValue(_report_interval, "interval");
    getPIDSet(_pids, "pid");

    // By default, all PIDs are selected
    if (!present(u"pid")) {
        _pids.set();
    }

    // Create output file
    if (present(u"output-file")) {
        const std::string name (value(u"output-file"));
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

ts::ProcessorPlugin::Status ts::CountPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Check if the packet must be counted
    const PID pid = pkt.getPID();
    bool ok = _pids[pid];
    if (_negate) {
        ok = !ok;
    }

    // Process reporting intervals.
    if (_report_interval > 0) {
        if (_current_pkt == 0) {
            // Set initial interval
            _last_report.start = Time::CurrentUTC();
            _last_report.counted_packets = 0;
            _last_report.total_packets = 0;
        }
        else if (_current_pkt % _report_interval == 0) {
            // It is time to produce a report.
            // Get current state.
            IntervalReport now;
            now.start = Time::CurrentUTC();
            now.total_packets = _current_pkt;
            now.counted_packets = 0;
            for (size_t p = 0; p < PID_MAX; p++) {
                now.counted_packets += _counters[p];
            }

            // Compute bitrates.
            const MilliSecond duration = now.start - _last_report.start;
            BitRate counted = 0;
            BitRate total = 0;
            if (duration > 0) {
                counted = PacketBitRate(now.counted_packets - _last_report.counted_packets, duration);
                total = PacketBitRate(now.total_packets - _last_report.total_packets, duration);
            }
            const UString sTime1(Time::CurrentLocalTime());
            const std::string sTime(sTime1.toUTF8()); //@@@@
            const std::string sCountedPackets(Decimal(now.counted_packets));
            const std::string sCountedBitrate(Decimal(counted));
            const std::string sTotalPackets(Decimal(now.total_packets));
            const std::string sTotalBitrate(Decimal(total));
            report("%s, counted: %s packets, %s b/s, total: %s packets, %s b/s",
                   sTime.c_str(), sCountedPackets.c_str(), sCountedBitrate.c_str(), sTotalPackets.c_str(), sTotalBitrate.c_str());

            // Save current report.
            _last_report = now;
        }
    }

    // Report packets
    if (ok) {
        if (_report_all) {
            if (_brief_report) {
                report("%" FMT_INT64 "u %d", _current_pkt, int (pid));
            }
            else {
                const std::string curpkt(Decimal(_current_pkt));
                report("Packet: %10s, PID: %4d (0x%04X)", curpkt.c_str(), int (pid), int (pid));
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
