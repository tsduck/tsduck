//----------------------------------------------------------------------------
//
// TSDuck sample application running a chain of plugins.
// One of the plugins is a custom one, in this source file.
// The use of plugin events is also illustrated.
// Most applications do not need custom plugins and use only standard ones.
//
// Notes:
//   - With TSDuck 3.21 and below, use export TSPLUGINS_PATH=/usr/bin
//   - With TSDuck 3.22 and above, use export TSPLUGINS_PATH=/usr/lib/tsduck
//   - Use /usr/local instead of /usr on macOS.
//   - On some 64-bit systems, use lib64 instead of lib.
//
//----------------------------------------------------------------------------

#include "tsduck.h"


//----------------------------------------------------------------------------
// Plugin-specific data type used during event signalling.
// Probably not useful in many applications, just to provide sample code.
//----------------------------------------------------------------------------

class FooBarData : public ts::Object
{
public:
    FooBarData(const ts::UString& s = ts::UString()) : message(s) {}
    ts::UString message;
};



//----------------------------------------------------------------------------
// A sample custom packet processing plugin.
// The plugin takes one optional PID parameter and counts packets in that PID.
// Most application don't need specific plugins, just to provide sample code.
//----------------------------------------------------------------------------

class FooBarPlugin: public ts::ProcessorPlugin
{
public:
    // Constructor.
    FooBarPlugin(ts::TSP*);

    // Implementation of plugin API. Not all methods are required.
    virtual bool getOptions() override;
    virtual bool start() override;
    virtual bool stop() override;
    virtual Status processPacket(ts::TSPacket&, ts::TSPacketMetadata&) override;

    // A factory static method which creates an instance of that class.
    // This method is used to register the plugin in the repository.
    static ts::ProcessorPlugin* CreateInstance(ts::TSP*);

private:
    // Command line options:
    ts::PID _pid;   // a PID to count

    // Working data:
    ts::PacketCounter _count;  // number of packets of the PID
};

// A factory static method which creates an instance of that class.
ts::ProcessorPlugin* FooBarPlugin::CreateInstance(ts::TSP* t)
{
    return new FooBarPlugin(t);
}

// Custom plugin constructor.
FooBarPlugin::FooBarPlugin(ts::TSP* t) :
    ts::ProcessorPlugin(t, u"Count TS packets in one PID", u"[options]"),
    _pid(ts::PID_NULL),
    _count(0)
{
    // Declare command line options.
    option(u"pid", 'p', PIDVAL);
    help(u"pid", u"The PID to select.");
}

// Get option values from the command line, after command line analysis.
bool FooBarPlugin::getOptions()
{
    _pid = intValue<ts::PID>(u"pid", ts::PID_NULL);
    return true;
}

// Called each time the plugin is started.
bool FooBarPlugin::start()
{
    _count = 0;
    return true;
}

// Called each time the plugin is stopped.
bool FooBarPlugin::stop()
{
    tsp->info(u"PID: 0x%X (%d), packets: %'d", {_pid, _pid, _count});
    return true;
}

// Called every packet in the stream.
ts::ProcessorPlugin::Status FooBarPlugin::processPacket(ts::TSPacket& pkt, ts::TSPacketMetadata& metadata)
{
    if (pkt.getPID() == _pid) {
        // Count packets in the specified PID.
        _count++;

        // Signal an event to the application.
        // Nothing useful here, this is just to illustrate the feature.
        // The event code is plugin-specific, just use 0xDEADBEEF as example.
        FooBarData data(u"hello from processPacket()");
        tsp->signalPluginEvent(0xDEADBEEF, &data);
    }
    return TSP_OK;
}


//----------------------------------------------------------------------------
// A plugin event handler. Invoked each time a plugin signals an event.
//----------------------------------------------------------------------------

class FooBarHandler : public ts::PluginEventHandlerInterface
{
public:
    FooBarHandler() = delete;
    FooBarHandler(ts::Report& report) : _report(report) {}
    virtual void handlePluginEvent(const ts::PluginEventContext& context) override;
private:
    ts::Report& _report;
};

void FooBarHandler::handlePluginEvent(const ts::PluginEventContext& ctx)
{
    FooBarData* data = dynamic_cast<FooBarData*>(ctx.pluginData());
    if (data != nullptr) {
        _report.info(u"[HANDLER] plugin: %s, event code: 0x%X, packets: %'d, application message: %s", {
                     ctx.pluginName(),
                     ctx.eventCode(),
                     ctx.pluginPackets(),
                     data->message});
    }
}


//----------------------------------------------------------------------------
// Application entry point.
//----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // Register our custom plugin with the name "foobar".
    ts::PluginRepository::Instance()->registerProcessor(u"foobar", FooBarPlugin::CreateInstance);

    // Use an asynchronous logger to report errors, logs, debug, etc.
    // Make it display all messages up to debug level (default is info level).
    ts::AsyncReport report(ts::Severity::Debug);

    // Build tsp options. Accept most default values, except a few ones.
    ts::TSProcessorArgs opt;
    opt.app_name = u"myexec";  // for error messages only.
    opt.monitor = true;        // use a system monitor threads.
    opt.instuff_start = 10;    // insert 10 null packets at start of stream.
    opt.instuff_stop = 5;      // insert 5 null packets at end of stream.

    // Use "http" input plugin, using a small TS file from the TSDuck stream repository.
    // Repeat the file twice.
    opt.input = {u"http", {u"--repeat", u"2", u"https://tsduck.io/streams/test-patterns/test-3packets-04-05-06.ts"}};

    // Use a list of packet processing plugins.
    // Some plugins are standard, from shared libraries.
    // One plugin is our custom one.
    opt.plugins = {
        {u"pattern", {u"--pid", u"4", u"DEADBEEF"}},
        {u"foobar", {u"--pid", u"5"}},
        {u"continuity", {}},
    };

    // Use "file" output plugin to store the result in a local file.
    opt.output = {u"file", {u"output.ts"}};

    // The TS processing is performed into this object.
    ts::TSProcessor tsproc(report);

    // Register an event handler for plugins.
    FooBarHandler handler(report);
    tsproc.registerEventHandler(&handler);

    // Start the TS processing.
    if (!tsproc.start(opt)) {
        return EXIT_FAILURE;
    }

    // And wait for TS processing termination.
    tsproc.waitForTermination();
    return EXIT_SUCCESS;
}
