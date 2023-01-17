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
//  Report various statistics on PID's and labels.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsTSSpeedMetrics.h"
#include "tsSingleDataStatistics.h"
#include "tsSafePtr.h"
#include "tsFatal.h"
#include "tsFileNameGenerator.h"
#include <cmath>


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class StatsPlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(StatsPlugin);
    public:
        // Implementation of plugin API
        StatsPlugin (TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Each category of packets (PID or lable) is described by a structure like this.
        // The map is indexed by PID or label.
        class Context;
        typedef SafePtr<Context> ContextPtr;
        typedef std::map<size_t, ContextPtr> ContextMap;

        // Command line options.
        bool       _track_pids;        // Track PID's, not labels.
        bool       _log;               // Report statistics through the logger, not files.
        bool       _csv;               // Use CSV format for statistics.
        bool       _header;            // Display header lines.
        bool       _multiple_output;   // Don't rewrite output files with --interval.
        UString    _csv_separator;     // Separator character in CSV lines.
        UString    _output_name;       // Output file name.
        NanoSecond _output_interval;   // Recreate output at this time interval.
        PIDSet     _pids;              // List of PID's to track.
        TSPacketLabelSet _labels;      // List of labels to track.

        // Working data.
        std::ofstream     _output_stream;  // Output file stream.
        std::ostream*     _output;         // Point to actual output stream.
        ContextMap        _ctx_map;        // Description of all tracked categories of packets.
        TSSpeedMetrics    _metrics;        // Timing to synchronize next output files.
        NanoSecond        _next_report;    // Next time to create next output.
        FileNameGenerator _name_gen;       // Generate multiple output file names.

        // Get or create the description of a tracked PID or label.
        ContextPtr getContext(size_t index);

        // Open, close and create statistics report.
        bool openOutput();
        void closeOutput();
        bool produceReport();

        // Description of a tracked category of packet (PID or label).
        class Context
        {
        public:
            uint64_t total_pkt;      // Total number of packets in that category.
            uint64_t last_ts_index;  // Index in TS of last packet of the category.
            SingleDataStatistics<uint64_t> ipkt; // Inter-packet distance statistics.

            // Constructor.
            Context();

            // Add packet data to the context.
            void addPacketData(PacketCounter, const TSPacket&);
        };
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"stats", ts::StatsPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::StatsPlugin::StatsPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Report various statistics on PID's and labels", u"[options]"),
    _track_pids(true),
    _log(false),
    _csv(false),
    _header(false),
    _multiple_output(false),
    _csv_separator(TS_DEFAULT_CSV_SEPARATOR),
    _output_name(),
    _output_interval(0),
    _pids(),
    _labels(),
    _output_stream(),
    _output(nullptr),
    _ctx_map(),
    _metrics(),
    _next_report(0),
    _name_gen()
{
    option(u"csv", 'c');
    help(u"csv",
         u"Report the statistics in CSV (comma-separated values) format. "
         u"All values are reported in decimal. "
         u"It is suitable for later analysis using tools such as Microsoft Excel.");

    option(u"interval", 'i', POSITIVE);
    help(u"interval", u"seconds",
         u"Produce a new output file at regular intervals. "
         u"The interval value is in seconds. "
         u"After outputting a file, the statistics are reset, "
         u"ie. each output file contains a fully independent analysis.");

    option(u"label", 'l', INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketLabelSet::MAX);
    help(u"label", u"label1[-label2]",
         u"Analyze packets with the specified label or range of labels. "
         u"Several -l or --label options may be specified. "
         u"By default, all PID's are analyzed. "
         u"The options --label and --pid are mutually exclusive.");

    option(u"log");
    help(u"log",
         u"Report the statistics in the common transport stream logger, not in a file.");

    option(u"multiple-files", 'm');
    help(u"multiple-files",
         u"When used with --interval and --output-file, create a new file for each "
         u"statistics report instead of rewriting the previous file. "
         u"Assuming that the specified output file name has the form 'base.ext', "
         u"each file is created with a time stamp in its name as 'base-YYYYMMDD-hhmmss.ext'.");

    option(u"noheader", 'n');
    help(u"noheader",
         u"Do not output initial header line in CSV and text format.");

    option(u"output-file", 'o', FILENAME);
    help(u"output-file", u"filename",
         u"Specify the output text file for the analysis result. "
         u"By default, use the standard output.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Analyze the specified PID or range of PID's. "
         u"Several -p or --pid options may be specified. "
         u"By default, all PID's are analyzed.");

    option(u"separator", 's', STRING);
    help(u"separator", u"string",
         u"Field separator string in CSV output (default: '" TS_DEFAULT_CSV_SEPARATOR u"').");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::StatsPlugin::getOptions()
{
    _log = present(u"log");
    _csv = present(u"csv");
    _header = !present(u"noheader");
    _multiple_output = present(u"multiple-files");
    _output_interval = NanoSecPerSec * intValue<Second>(u"interval", 0);
    getValue(_csv_separator, u"separator", TS_DEFAULT_CSV_SEPARATOR);
    getValue(_output_name, u"output-file");
    getIntValues(_pids, u"pid");
    getIntValues(_labels, u"label");

    if (_pids.any() && _labels.any()) {
        tsp->error(u"options --pid and --label are mutually exclusive");
        return false;
    }
    if (_pids.none() && _labels.none()) {
        // Default: analyze all PID's.
        _pids.set();
    }
    if (_log && !_output_name.empty()) {
        tsp->error(u"options --log and --output-file are mutually exclusive");
        return false;
    }

    _track_pids = _pids.any();
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::StatsPlugin::start()
{
    // For production of multiple reports at regular intervals.
    _metrics.start();
    _next_report = _output_interval;
    _name_gen.initDateTime(_output_name);

    // Create the output file. Note that this file is used only in the stop
    // method and could be created there. However, if the file cannot be
    // created, we do not want to wait all along the analysis and finally fail.
    _output = _output_name.empty() ? &std::cout : &_output_stream;
    if (_output_interval == 0 && !openOutput()) {
        return false;
    }

    _ctx_map.clear();
    return true;
}


//----------------------------------------------------------------------------
// Create an output file. Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::StatsPlugin::openOutput()
{
    // Standard output is always open. Also do not reopen an open file.
    if (_log || _output_name.empty() || _output_stream.is_open()) {
        return true;
    }

    // Build file name in case of --multiple-files
    const UString name(_multiple_output ? _name_gen.newFileName() : _output_name);

    // Create the file
    _output_stream.open(name.toUTF8().c_str());
    if (_output_stream) {
        tsp->verbose(u"created %s", {name});
        return true;
    }
    else {
        tsp->error(u"cannot create file %s", {name});
        return false;
    }
}


//----------------------------------------------------------------------------
// Close current output file.
//----------------------------------------------------------------------------

void ts::StatsPlugin::closeOutput()
{
    if (!_output_name.empty() && _output_stream.is_open()) {
        _output_stream.close();
    }
}


//----------------------------------------------------------------------------
// Produce a report. Return true on success, false on error.
//----------------------------------------------------------------------------

bool ts::StatsPlugin::produceReport()
{
    // Create output file if required.
    if (!openOutput()) {
        return false;
    }

    std::ostream& out(*_output);
    const UString name(_track_pids ? u"PID" : u"Label");

    // Header lines if necessary.
    if (_header && !_log) {
        if (_csv) {
            out << name << _csv_separator
                << "Total" << _csv_separator
                << "IPD min" << _csv_separator
                << "IPD max" << _csv_separator
                << "IPD mean" << _csv_separator
                << "IPD std dev" << std::endl;
        }
        else {
            out << "          Total nb  ......Inter-packet distance......." << std::endl
                << name.toJustifiedLeft(6)
                <<       "  of packets     min     max      mean   std dev" << std::endl
                << "------  ----------  ------  ------  --------  --------" << std::endl;
        }
    }

    // Loop on all categories.
    for (const auto& it : _ctx_map) {

        // PID context.
        const size_t index = it.first;
        const Context& ctx(*(it.second));

        if (_log) {
            tsp->info(u"%s: 0x%X  Total: %8'd  IPD min: %3d  max: %5d  mean: %s  std-dev: %s",
                      {name, index, ctx.total_pkt, ctx.ipkt.minimum(), ctx.ipkt.maximum(), ctx.ipkt.meanString(7), ctx.ipkt.standardDeviationString(7)});
        }
        else if (_csv) {
            out << index << _csv_separator
                << ctx.total_pkt << _csv_separator
                << ctx.ipkt.minimum() << _csv_separator
                << ctx.ipkt.maximum() << _csv_separator
                << ctx.ipkt.meanString() << _csv_separator
                << ctx.ipkt.standardDeviationString() << std::endl;
        }
        else {
            out << UString::Format(_track_pids ? u"0x%04X" : u"%-6d", {index})
                << UString::Format(u"  %10'd  %6d  %6d  %s  %s", {ctx.total_pkt, ctx.ipkt.minimum(), ctx.ipkt.maximum(), ctx.ipkt.meanString(8), ctx.ipkt.standardDeviationString(8)})
                << std::endl;
        }
    }

    // Close output file if required.
    closeOutput();
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::StatsPlugin::stop()
{
    produceReport();
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::StatsPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // Check tracked pids.
    if (_pids.test(pid)) {
        const ContextPtr ctx(getContext(pid));
        ctx->addPacketData(tsp->pluginPackets(), pkt);
    }

    // Check tracked labels.
    if (!_track_pids) {
        for (size_t label = 0; label < _labels.size(); ++label) {
            if (pkt_data.hasLabel(label)) {
                const ContextPtr ctx(getContext(label));
                ctx->addPacketData(tsp->pluginPackets(), pkt);
            }
        }
    }

    // With --interval, check if it is time to produce a report
    if (_output_interval > 0 && _metrics.processedPacket() && _metrics.sessionNanoSeconds() >= _next_report) {
        // Time to produce a report.
        if (!produceReport()) {
            return TSP_END;
        }
        // Reset analysis context.
        _ctx_map.clear();
        // Compute next report time.
        _next_report += _output_interval;
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Get or create the description of a tracked PID.
//----------------------------------------------------------------------------

ts::StatsPlugin::ContextPtr ts::StatsPlugin::getContext(size_t index)
{
    const auto it = _ctx_map.find(index);
    if (it != _ctx_map.end()) {
        return it->second;
    }
    else {
        ContextPtr ptr(new Context);
        CheckNonNull(ptr.pointer());
        _ctx_map[index] = ptr;
        return ptr;
    }
}


//----------------------------------------------------------------------------
// Constructor of a tracked PID context.
//----------------------------------------------------------------------------

ts::StatsPlugin::Context::Context() :
    total_pkt(0),
    last_ts_index(0),
    ipkt()
{
}


//----------------------------------------------------------------------------
// Add packet data to the context.
//----------------------------------------------------------------------------

void ts::StatsPlugin::Context::addPacketData(PacketCounter ts_index, const TSPacket& pkt)
{
    // Accumulate inter-packet statistics, starting at the second packet.
    if (total_pkt > 0) {
        ipkt.feed(ts_index - last_ts_index);
    }

    // Global packet statistics.
    total_pkt++;
    last_ts_index = ts_index;
}
