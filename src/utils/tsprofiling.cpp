//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
// Profiling and benchmark environment for transport stream processor.
// Same command line syntax as tsp.
//
// Rationale: Using tsp to debug plugins is ok. But when it comes to
// profiling, a heavily multi-threaded application such as tsp is not
// convenient. Profiling tools are very bad with multi-threaded applications.
// This test program does the same as tsp but in the main thread. This
// is completely inappropriate for production and should be reserved to
// plugin profiling or debugging.
//
// Limitations:
// - Awful performances.
// - No support for joint termination.
// - Non-exhaustive error processing.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsArgsWithPlugins.h"
#include "tsDuckContext.h"
#include "tsPCRAnalyzer.h"
#include "tsPluginRepository.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
// Command line options
//----------------------------------------------------------------------------

namespace {
    class Options: public ts::ArgsWithPlugins
    {
        TS_NOBUILD_NOCOPY(Options);
    public:
        Options(int argc, char *argv[]);

        ts::DuckContext         duck {this};
        size_t                  buffer_size = 0;
        ts::BitRate             fixed_bitrate = 0;
        ts::PluginOptions       input {};
        ts::PluginOptionsVector plugins {};
        ts::PluginOptions       output {};
    };
}

Options::Options(int argc, char *argv[]) :
    ts::ArgsWithPlugins(0, 1, 0, UNLIMITED_COUNT, 0, 1, u"Mono-thread profiling and debugging environment for tsp plugins", u"[options]")
{
    duck.defineArgsForCAS(*this);
    duck.defineArgsForCharset(*this);
    duck.defineArgsForHFBand(*this);
    duck.defineArgsForPDS(*this);
    duck.defineArgsForFixingPDS(*this);
    duck.defineArgsForTimeReference(*this);
    duck.defineArgsForStandards(*this);

    option<ts::BitRate>(u"bitrate", 'b');
    help(u"bitrate", u"Specify the input bitrate.");

    option(u"packet-buffer", 'p', POSITIVE);
    help(u"packet-buffer", u"Specify the maximum number of TS packets in the buffer. The default is 1000.");

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    duck.loadArgs(*this);
    getIntValue(buffer_size, u"packet-buffer", 1000);
    getValue(fixed_bitrate, u"bitrate");
    getPlugin(input, ts::PluginType::INPUT, u"file");
    getPlugin(output, ts::PluginType::OUTPUT, u"drop");
    getPlugins(plugins, ts::PluginType::PROCESSOR);

    // Final checking
    exitOnError();
}


//----------------------------------------------------------------------------
// Plugin executor class.
//----------------------------------------------------------------------------

namespace {
    class PluginExecutor: public ts::TSP
    {
        TS_NOBUILD_NOCOPY(PluginExecutor);
    public:
        // Constructors and destructors.
        PluginExecutor(Options& opt, size_t index, PluginExecutor* previous);
        virtual ~PluginExecutor() override;

        // Implementation of TSP virtual methods.
        virtual ts::UString pluginName() const override { return _name; }
        virtual ts::Plugin* plugin() const override { return _shlib; }
        virtual size_t pluginIndex() const override { return _index; }
        virtual size_t pluginCount() const override { return _opt.plugins.size() + 2; }
        virtual void signalPluginEvent(uint32_t, ts::Object*) const override {}
        virtual void useJointTermination(bool on) override {}
        virtual void jointTerminate() override {}
        virtual bool useJointTermination() const override { return false; }
        virtual bool thisJointTerminated() const override { return false; }

    protected:
        Options& _opt;              // Application options.
        bool _own_bitrate = false;  // This plugin manages its own bitrate (ie. does not get it from previous plugin).

        // Update bitrate from previous plugin executor and from current plugin instance.
        void updateBitrateFromPrevious();
        void updateBitrateFromCurrent();

    private:
        size_t          _index = 0;           // Plugin index in the chain.
        ts::UString     _name {};             // Plugin name.
        ts::Plugin*     _shlib = nullptr;     // Plugin instance.
        PluginExecutor* _previous = nullptr;  // Previous plugin executor.
    };
}

// Constructor: allocate and start the plugin.
PluginExecutor::PluginExecutor(Options& opt, size_t index, PluginExecutor* previous) :
    ts::TSP(opt.maxSeverity(), ts::UString(), &opt),
    _opt(opt),
    _index(index),
    _previous(previous)
{
    const ts::UStringVector* args = nullptr;
    const ts::UChar* shell_opt = nullptr;

    // Create the plugin instance object
    if (_index == 0) {
        // Input plugin.
        shell_opt = u" -I";
        _name = _opt.input.name;
        args = &_opt.input.args;
        ts::PluginRepository::InputPluginFactory allocator = ts::PluginRepository::Instance().getInput(_name, _opt);
        if (allocator != nullptr) {
            _shlib = allocator(this);
        }
    }
    else if (_index <= _opt.plugins.size()) {
        // Packet processing plugin.
        shell_opt = u" -P";
        _name = _opt.plugins[_index - 1].name;
        args = &_opt.plugins[_index - 1].args;
        ts::PluginRepository::ProcessorPluginFactory allocator = ts::PluginRepository::Instance().getProcessor(_name, _opt);
        if (allocator != nullptr) {
            _shlib = allocator(this);
        }
    }
    else if (_index == _opt.plugins.size() + 1) {
        // Output plugin.
        shell_opt = u" -O";
        _name = _opt.output.name;
        args = &_opt.output.args;
        ts::PluginRepository::OutputPluginFactory allocator = ts::PluginRepository::Instance().getOutput(_name, _opt);
        if (allocator != nullptr) {
            _shlib = allocator(this);
        }
    }
    if (_shlib == nullptr) {
        // Error message already displayed.
        return;
    }

    // Prefix messages with plugin name.
    setReportPrefix(_name + u": ");

    // Configure plugin object.
    _shlib->setShell(_opt.appName() + shell_opt);
    _shlib->setMaxSeverity(_opt.maxSeverity());

    // Submit the plugin arguments for analysis.
    // Do not process argument redirection, already done at tsp command level.
    _shlib->analyze(_name, *args, false);

    // The process should have terminated on argument error.
    assert(_shlib->valid());

    // Load arguments and start the plugin.
    if (!_shlib->getOptions() || !_shlib->start()) {
        _opt.error(u"error starting plugin %s", _name);
    }
}

// Destructor: cleanup the plugin.
PluginExecutor::~PluginExecutor()
{
    if (_shlib != nullptr) {
        delete _shlib;
        _shlib = nullptr;
    }
}

// Update bitrate from previous plugin executor.
void PluginExecutor::updateBitrateFromPrevious()
{
    if (!_own_bitrate && _previous != nullptr) {
        _tsp_bitrate = _previous->_tsp_bitrate;
    }
}

// Update bitrate from previous plugin executor and from current plugin instance..
void PluginExecutor::updateBitrateFromCurrent()
{
    const ts::BitRate bitrate = _shlib->getBitrate();
    if (bitrate != 0) {
        _tsp_bitrate = bitrate;
        _tsp_bitrate_confidence = _shlib->getBitrateConfidence();
        _own_bitrate = true;
    }
}


//----------------------------------------------------------------------------
// Input plugin executor class.
//----------------------------------------------------------------------------

namespace {
    class InputPluginExecutor: public PluginExecutor
    {
        TS_NOBUILD_NOCOPY(InputPluginExecutor);
    public:
        // Constructor.
        InputPluginExecutor(Options& opt);

        // Implementation of TSP virtual methods.
        virtual ts::InputPlugin* plugin() const override { return dynamic_cast<ts::InputPlugin*>(PluginExecutor::plugin()); }

        // Receive packets.
        size_t receive(ts::TSPacket* buffer, ts::TSPacketMetadata* pkt_data, size_t max_packets);

    private:
        bool              _sync_lost;
        ts::PacketCounter _next_get_bitrate;
        ts::PCRAnalyzer   _pcr_analyzer;
    };
}

// Constructor.
InputPluginExecutor::InputPluginExecutor(Options& opt) :
    PluginExecutor(opt, 0, nullptr),
    _sync_lost(false),
    _next_get_bitrate(0),
    _pcr_analyzer()
{
    _tsp_bitrate = _opt.fixed_bitrate;
}

// Receive packets.
size_t InputPluginExecutor::receive(ts::TSPacket* packets, ts::TSPacketMetadata* metadata, size_t max_packets)
{
    // Receive packets from the plugin. End of stream after loss of synchronization.
    size_t count = _sync_lost ? 0 : plugin()->receive(packets, metadata, max_packets);
    if (count == 0) {
        return 0;
    }

    // Validate sync byte (0x47) at beginning of each packet
    for (size_t n = 0; n < count; ++n) {
        if (packets[n].hasValidSync()) {
            addPluginPackets(1);
            if (_pcr_analyzer.feedPacket(packets[n]) && _tsp_bitrate == 0 && !_own_bitrate) {
                // First valid bitrate from PCR.
                _tsp_bitrate = _pcr_analyzer.bitrate188();
            }
        }
        else {
            error(u"synchronization lost after %'d packets, got 0x%X instead of 0x%X", pluginPackets(), packets[n].b[0], ts::SYNC_BYTE);
            _sync_lost = true;
            count = n;
        }
    }

    // Process periodic bitrate adjustment.
    if (_opt.fixed_bitrate == 0 && pluginPackets() >= _next_get_bitrate) {
        ts::BitRate bitrate = plugin()->getBitrate();
        if (bitrate > 0) {
            _tsp_bitrate = bitrate;
            _tsp_bitrate_confidence = plugin()->getBitrateConfidence();
            _own_bitrate = true;
        }
        else if (!_own_bitrate && _pcr_analyzer.bitrateIsValid()) {
            _tsp_bitrate = _pcr_analyzer.bitrate188();
        }
        _next_get_bitrate = pluginPackets() + 1000; // arbitrary...
    }
    return count;
}


//----------------------------------------------------------------------------
// Packet processor plugin executor class.
//----------------------------------------------------------------------------

namespace {
    class ProcessorPluginExecutor: public PluginExecutor
    {
        TS_NOBUILD_NOCOPY(ProcessorPluginExecutor);
    public:
        // Constructor.
        ProcessorPluginExecutor(Options& opt, size_t index, PluginExecutor* previous);

        // Implementation of TSP virtual methods.
        virtual ts::ProcessorPlugin* plugin() const override { return dynamic_cast<ts::ProcessorPlugin*>(PluginExecutor::plugin()); }

        // Process packets.
        bool process(ts::TSPacket* packets, ts::TSPacketMetadata* metadata, size_t count);
    };
}

// Constructor.
ProcessorPluginExecutor::ProcessorPluginExecutor(Options& opt, size_t index, PluginExecutor* next) :
    PluginExecutor(opt, index, next)
{
}

// Process packets.
bool ProcessorPluginExecutor::process(ts::TSPacket* packets, ts::TSPacketMetadata* metadata, size_t count)
{
    // Propagate bitrate if needed.
    updateBitrateFromPrevious();

    // Loop on packets.
    for (size_t i = 0; i < count; ++i) {
        if (packets[i].b[0] == 0) {
            // The packet has already been dropped by a previous packet processor.
            addNonPluginPackets(1);
        }
        else {
            metadata[i].setBitrateChanged(false);
            switch (plugin()->processPacket(packets[i], metadata[i])) {
                case ts::ProcessorPlugin::TSP_END:
                    return false;
                case ts::ProcessorPlugin::TSP_DROP:
                    packets[i].b[0] = 0;
                    addNonPluginPackets(1);
                    break;
                case ts::ProcessorPlugin::TSP_NULL:
                    packets[i] = ts::NullPacket;
                    addPluginPackets(1);
                    break;
                case ts::ProcessorPlugin::TSP_OK:
                    addPluginPackets(1);
                    break;
                default:
                    break;
            }
            if (metadata[i].getBitrateChanged()) {
                updateBitrateFromCurrent();
            }
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Output plugin executor class.
//----------------------------------------------------------------------------

namespace {
    class OutputPluginExecutor: public PluginExecutor
    {
        TS_NOBUILD_NOCOPY(OutputPluginExecutor);
    public:
        // Constructor.
        OutputPluginExecutor(Options& opt, PluginExecutor* previous);

        // Implementation of TSP virtual methods.
        virtual ts::OutputPlugin* plugin() const override { return dynamic_cast<ts::OutputPlugin*>(PluginExecutor::plugin()); }

        // Send packets.
        bool send(ts::TSPacket* packets, ts::TSPacketMetadata* metadata, size_t count);
    };
}

// Constructor.
OutputPluginExecutor::OutputPluginExecutor(Options& opt, PluginExecutor* previous) :
    PluginExecutor(opt, opt.plugins.size() + 1, previous)
{
}

// Send packets.
bool OutputPluginExecutor::send(ts::TSPacket* packets, ts::TSPacketMetadata* metadata, size_t count)
{
    // Propagate bitrate if needed.
    updateBitrateFromPrevious();

    // Loop on chunks of non-dropped packets.
    size_t chunk_start = 0;
    while (chunk_start < count) {
        // Locate next chunk of non-dropped packets.
        while (chunk_start < count && packets[chunk_start].b[0] != ts::SYNC_BYTE) {
            chunk_start++;
        }
        size_t chunk_end = chunk_start;
        while (chunk_end < count && packets[chunk_end].b[0] == ts::SYNC_BYTE) {
            chunk_end++;
        }
        // Output chunk of packets.
        if (chunk_end > chunk_start && !plugin()->send(packets + chunk_start, metadata + chunk_start, chunk_end - chunk_start)) {
            return false;
        }
        chunk_start = chunk_end;
    }

    return true;
}


//----------------------------------------------------------------------------
// Program main code.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Get command line options.
    Options opt(argc, argv);
    CERR.setMaxSeverity(opt.maxSeverity());

    // Prevent from being killed when writing on broken pipes.
    ts::IgnorePipeSignal();

    // Allocate and start all plugins.
    InputPluginExecutor* input = new InputPluginExecutor(opt);
    PluginExecutor* previous = input;
    std::vector<ProcessorPluginExecutor*> procs;
    for (size_t i = 0; i < opt.plugins.size(); ++i) {
        ProcessorPluginExecutor* plugin = new ProcessorPluginExecutor(opt, i + 1, previous);
        procs.push_back(plugin);
        previous = plugin;
    }
    OutputPluginExecutor* output = new OutputPluginExecutor(opt, previous);

    // Exit on error when initializing the plugins.
    if (opt.gotErrors()) {
        return EXIT_FAILURE;
    }

    // Packet buffers.
    ts::TSPacketVector packets(opt.buffer_size);
    ts::TSPacketMetadataVector metadata(opt.buffer_size);

    bool success = true;
    size_t received = 0;

    // Now loop on plugins, sequentially.
    while (success && (received = input->receive(packets.data(), metadata.data(), packets.size())) != 0) {
        // Received a chunk of packets, loop on plugins.
        for (size_t pli = 0; success && pli < procs.size(); ++pli) {
            success = procs[pli]->process(packets.data(), metadata.data(), received);
        }
        success = success && output->send(packets.data(), metadata.data(), received);
        ts::TSPacketMetadata::Reset(metadata.data(), received);
    }

    // Close and deallocate all plugins.
    input->plugin()->stop();
    delete input;
    for (size_t i = 0; i < opt.plugins.size(); ++i) {
        procs[i]->plugin()->stop();
        delete procs[i];
    }
    output->plugin()->stop();
    delete output;
    return EXIT_SUCCESS;
}
