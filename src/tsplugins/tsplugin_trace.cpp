//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Trace packets with a custom message.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"

#define DEFAULT_FORMAT u"Packet: %i, PID: %P (%p)"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class TracePlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(TracePlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options
        UString          _format {};        // Message format
        PIDSet           _pids {};          // Trace packets in these PID's
        TSPacketLabelSet _labels {};        // Trace packets with any of these labels
        fs::path         _outfile_name {};  // Output file name

        // Working data
        std::ofstream _outfile {};          // User-specified output file
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"trace", ts::TracePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TracePlugin::TracePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Trace packets with a custom message", u"[options]")
{
    option(u"format", 'f', STRING);
    help(u"format", u"'string'",
         u"Specify the format of trace lines. "
         u"The fields with a % sign are replaced by the corresponding value:\n"
         u"- %p : PID value.\n"
         u"- %i : index of the packet as seen by the plugin.\n"
         u"- %a : index in the input stream, including removed packets.\n"
         u"When the letter after % is lowercase, the value is displayed in decimal. "
         u"When it is uppercase, the value is displayed in hexadecimal. "
         u"Use %% for a literal '%' sign. "
         u"The default format is '" DEFAULT_FORMAT "'.");

    option(u"label", 'l', INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketLabelSet::MAX);
    help(u"label", u"label1[-label2]",
         u"Trace packets with any of these label values. "
         u"Labels should have typically been set by a previous plugin in the chain. "
         u"Several --label options may be specified.\n\n"
         u"Note that the option --label is different from the generic option --only-label. "
         u"The generic option --only-label acts at tsp level and controls which packets are "
         u"passed to the plugin. All other packets are directly passed to the next plugin "
         u"without going through this plugin. The option --label, on the other hand, "
         u"is specific to the trace plugin and selects packets with specific labels "
         u"among the packets which are passed to this plugin.");

    option(u"output-file", 'o', FILENAME);
    help(u"output-file", u"filename",
         u"Specify the output file for reporting trace lines. "
         u"By default, report trace lines on standard error using the tsp logging mechanism.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Trace packets with these PID values. "
         u"By default, when no option --label or --pid is specified, all packets are traced. "
         u"Several --pid options may be specified.");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::TracePlugin::getOptions()
{
    getValue(_format, u"format", DEFAULT_FORMAT);
    getIntValues(_pids, u"pid");
    getIntValues(_labels, u"label");
    getPathValue(_outfile_name, u"output-file");
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::TracePlugin::start()
{
    // Create output file
    if (!_outfile_name.empty()) {
        verbose(u"creating %s", _outfile_name);
        _outfile.open(_outfile_name, std::ios::out);
        if (!_outfile) {
            error(u"cannot create %s", _outfile_name);
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::TracePlugin::stop()
{
    // Close output file
    if (_outfile.is_open()) {
        _outfile.close();
    }
    return true;
}




//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::TracePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // Filter packets.
    if ((_pids.any() || _labels.any()) && (!_pids.test(pid) && !pkt_data.hasAnyLabel(_labels))) {
        return TSP_OK;
    }

    // Build the trace message.
    UString line;
    bool percent = false;
    for (auto c : _format) {
        if (percent) {
            percent = false;
            switch (c) {
                case 'p':
                    line.format(u"%d", pid);
                    break;
                case 'P':
                    line.format(u"0x%X", pid);
                    break;
                case 'i':
                    line.format(u"%d", tsp->pluginPackets());
                    break;
                case 'I':
                    line.format(u"0x%08X", tsp->pluginPackets());
                    break;
                case 'a':
                    line.format(u"%d", tsp->totalPacketsInThread());
                    break;
                case 'A':
                    line.format(u"0x%08X", tsp->totalPacketsInThread());
                    break;
                case '%':
                    line.append(u'%');
                    break;
                default:
                    line.append(u'%');
                    line.append(c);
                    break;
            }
        }
        else if (c == u'%') {
            percent = true;
        }
        else {
            line.append(c);
        }
    }

    // Then report the message.
    if (_outfile.is_open()) {
        _outfile << line << std::endl;
    }
    else {
        info(line);
    }
    return TSP_OK;
}
