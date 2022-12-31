//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsPluginRepository.h"
#include "tsTime.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class CountPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(CountPlugin);
    public:
        // Implementation of plugin API
        CountPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

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

        // Command  line options:
        UString        _tag;                // Message tag
        bool           _negate;             // Negate filter (exclude selected packets)
        PIDSet         _pids;               // PID values to filter
        bool           _brief_report;       // Display biref report, values but not comments
        bool           _report_all;         // Report packet index and PID of each packet
        bool           _report_summary;     // Report summary
        bool           _report_total;       // Report total of all PIDs
        PacketCounter  _report_interval;    // If non-zero, report time-stamp at this packet interval

        // Working data:
        std::ofstream  _outfile;            // User-specified output file
        IntervalReport _last_report;        // Last report content
        PacketCounter  _counters[PID_MAX];  // Packet counter per PID

        // Report a line
        void report(const UChar* fmt, const std::initializer_list<ArgMixIn> args);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"count", ts::CountPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::CountPlugin::CountPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Count TS packets per PID", u"[options]"),
    _tag(),
    _negate(false),
    _pids(),
    _brief_report(false),
    _report_all(false),
    _report_summary(false),
    _report_total(false),
    _report_interval(0),
    _outfile(),
    _last_report(),
    _counters()
{
    option(u"all", 'a');
    help(u"all",
         u"Report packet index and PID for all packets from the selected PID's. "
         u"By default, only a final summary is reported.");

    option(u"brief", 'b');
    help(u"brief",
         u"Brief display. Report only the numerical values, not comment on their usage.");

    option(u"interval", 'i', UINT32);
    help(u"interval",
         u"Report a time-stamp and global packet count at regular intervals. The "
         u"specified value is a number of packets.");

    option(u"negate", 'n');
    help(u"negate",
         u"Negate the filter: specified PID's are excluded.");

    option(u"output-file", 'o', FILENAME);
    help(u"output-file", u"filename",
         u"Specify the output file for reporting packet counters. By default, report "
         u"on standard error using the tsp logging mechanism.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"PID filter: select packets with these PID values. Several -p or --pid "
         u"options may be specified. By default, if --pid is not specified, all "
         u"PID's are selected.");

    option(u"summary", 's');
    help(u"summary",
         u"Display a final summary of packet counts per PID. This is the default, "
         u"unless --all or --total is specified, in which case the final summary is "
         u"reported only if --summary is specified.");

    option(u"tag", 0, STRING);
    help(u"tag", u"'string'",
         u"Message tag to be displayed with count report lines. Useful when "
         u"the plugin is used several times in the same process.");

    option(u"total", 't');
    help(u"total",
         u"Display the total packet counts in all PID's.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::CountPlugin::getOptions()
{
    _report_all = present(u"all");
    _report_total = present(u"total");
    _report_summary = (!_report_all && !_report_total) || present(u"summary");
    _brief_report = present(u"brief");
    _negate = present(u"negate");
    getIntValue(_report_interval, u"interval");
    getIntValues(_pids, u"pid");
    _tag = value(u"tag");
    if (!_tag.empty()) {
        _tag += u": ";
    }

    // By default, all PIDs are selected
    if (!present(u"pid")) {
        _pids.set();
    }
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::CountPlugin::start()
{
    // Create output file
    if (present(u"output-file")) {
        const UString name(value(u"output-file"));
        tsp->verbose(u"creating %s", {name});
        _outfile.open(name.toUTF8().c_str(), std::ios::out);
        if (!_outfile) {
            tsp->error(u"cannot create %s", {name});
            return false;
        }
    }

    // Reset state
    TS_ZERO(_counters);

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
                    report(u"%d %d", {pid, _counters[pid]});
                }
                else {
                    report(u"%sPID %4d (0x%04X): %10'd packets", {_tag, pid, pid, _counters[pid]});
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
            report(u"%d", {total});
        }
        else {
            report(u"%stotal: counted %'d packets out of %'d", {_tag, total, tsp->pluginPackets()});
        }
    }

    // Close output file
    if (_outfile.is_open()) {
        _outfile.close();
    }

    return true;
}


//----------------------------------------------------------------------------
// Report a line
//----------------------------------------------------------------------------

void ts::CountPlugin::report(const UChar* fmt, const std::initializer_list<ArgMixIn> args)
{
    if (_outfile.is_open()) {
        _outfile << UString::Format(fmt, args) << std::endl;
    }
    else {
        tsp->info(fmt, args);
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::CountPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Check if the packet must be counted
    const PID pid = pkt.getPID();
    bool ok = _pids[pid];
    if (_negate) {
        ok = !ok;
    }

    // Process reporting intervals.
    if (_report_interval > 0) {
        if (tsp->pluginPackets() == 0) {
            // Set initial interval
            _last_report.start = Time::CurrentUTC();
            _last_report.counted_packets = 0;
            _last_report.total_packets = 0;
        }
        else if (tsp->pluginPackets() % _report_interval == 0) {
            // It is time to produce a report.
            // Get current state.
            IntervalReport now;
            now.start = Time::CurrentUTC();
            now.total_packets = tsp->pluginPackets();
            now.counted_packets = 0;
            for (size_t p = 0; p < PID_MAX; p++) {
                now.counted_packets += _counters[p];
            }

            // Compute bitrates.
            const MilliSecond duration = now.start - _last_report.start;
            BitRate countedBitRate = 0;
            BitRate totalBitRate = 0;
            if (duration > 0) {
                countedBitRate = PacketBitRate(now.counted_packets - _last_report.counted_packets, duration);
                totalBitRate = PacketBitRate(now.total_packets - _last_report.total_packets, duration);
            }
            report(u"%s%s, counted: %'d packets, %'d b/s, total: %'d packets, %'d b/s",
                   {_tag, Time::CurrentLocalTime(), now.counted_packets, countedBitRate, now.total_packets, totalBitRate});

            // Save current report.
            _last_report = now;
        }
    }

    // Report packets
    if (ok) {
        if (_report_all) {
            if (_brief_report) {
                report(u"%d %d", {tsp->pluginPackets(), pid});
            }
            else {
                report(u"%spacket: %10'd, PID: %4d (0x%04X)", {_tag, tsp->pluginPackets(), pid, pid});
            }
        }
        _counters[pid]++;
    }

    return TSP_OK;
}
