//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstspControlServer.h"
#include "tstspPluginExecutor.h"
#include "tsNullReport.h"
#include "tsReportBuffer.h"
#include "tsTelnetConnection.h"
#include "tsSysUtils.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tsp::ControlServer::ControlServer(TSProcessorArgs& options, Report& log, std::recursive_mutex& global_mutex, InputExecutor* input) :
    _options(options),
    _log(log.maxSeverity(), u"control commands: ", &log),
    _global_mutex(global_mutex),
    _input(input)
{
    // Locate output plugin, count packet processor plugins.
    if (_input != nullptr) {
        std::lock_guard<std::recursive_mutex> lock(_global_mutex);

        // The output plugin "precedes" the input plugin in the ring.
        _output = _input->ringPrevious<OutputExecutor>();
        assert(_output != nullptr);

        // Loop on all plugins between inputs and outputs
        PluginExecutor* proc = _input;
        while ((proc = proc->ringNext<PluginExecutor>()) != _output) {
            ProcessorExecutor* pe = dynamic_cast<ProcessorExecutor*>(proc);
            assert(pe != nullptr);
            _plugins.push_back(pe);
        }
    }
    _log.debug(u"found %d packet processor plugins", _plugins.size());

    // Register command handlers.
    _reference.setCommandLineHandler(this, &ControlServer::executeExit, u"exit");
    _reference.setCommandLineHandler(this, &ControlServer::executeSetLog, u"set-log");
    _reference.setCommandLineHandler(this, &ControlServer::executeList, u"list");
    _reference.setCommandLineHandler(this, &ControlServer::executeSuspend, u"suspend");
    _reference.setCommandLineHandler(this, &ControlServer::executeResume, u"resume");
    _reference.setCommandLineHandler(this, &ControlServer::executeRestart, u"restart");
}

ts::tsp::ControlServer::~ControlServer()
{
    // Terminate the thread and wait for actual thread termination.
    close();
    waitForTermination();
}


//----------------------------------------------------------------------------
// Start/stop the command receiver.
//----------------------------------------------------------------------------

bool ts::tsp::ControlServer::open()
{
    if (_options.control_port == 0) {
        // No control server, do nothing.
        return true;
    }
    else if (_is_open) {
        _log.error(u"tsp control command server alread started");
        return false;
    }
    else {
        // Open the TCP server.
        const IPSocketAddress addr(_options.control_local, _options.control_port);
        if (!_server.open(_options.control_local.generation(), _log) ||
            !_server.reusePort(_options.control_reuse, _log) ||
            !_server.bind(addr, _log) ||
            !_server.listen(5, _log))
        {
            _server.close(NULLREP);
            _log.error(u"error starting TCP server for control commands.");
            return false;
        }

        // Start the thread.
        _is_open = true;
        return start();
    }
}

void ts::tsp::ControlServer::close()
{
    if (_is_open) {
        // Close the TCP server. This will force the server thread to terminate.
        _terminate = true;
        _server.close(NULLREP);

        // Wait for the termination of the thread.
        waitForTermination();
        _is_open = false;
    }
}


//----------------------------------------------------------------------------
// Invoked in the context of the server thread.
//----------------------------------------------------------------------------

void ts::tsp::ControlServer::main()
{
    _log.debug(u"control command thread started");

    // Get accept errors in a buffer since some errors are normal.
    ReportBuffer<ThreadSafety::None> error(_log.maxSeverity());

    // Client address and connection.
    IPSocketAddress source;
    TCPConnection client;
    UString line;

    // Loop on incoming connections.
    // Since the commands are expected to be short, treat only one at a time.
    while (_server.accept(client, source, error)) {

        TelnetConnection telnet(client);

        // Filter allowed sources.
        // Set receive timeout on the connection and read one line.
        if (std::find(_options.control_sources.begin(), _options.control_sources.end(), IPAddress(source)) == _options.control_sources.end()) {
            _log.warning(u"connection attempt from unauthorized source %s (ignored)", source);
            telnet.sendLine("error: client address is not authorized", _log);
        }
        else if (client.setReceiveTimeout(_options.control_timeout, _log) && telnet.receiveLine(line, nullptr, _log)) {
            _log.verbose(u"received from %s: %s", source, line);

            // Reset the severity of the connection before analysing the line.
            // A previous analysis may have used --verbose or --debug.
            telnet.setMaxSeverity(Severity::Info);

            // Analyze the command, return errors on the client connection.
            if (_reference.processCommand(line, &telnet) != CommandStatus::SUCCESS) {
                telnet.error(u"invalid tsp control command: %s", line);
            }
        }

        client.closeWriter(_log);
        client.close(_log);
    }

    // If termination was requested, receive error is not an error.
    if (!_terminate && !error.empty()) {
        _log.error(error.messages());
    }
    _log.debug(u"control command thread completed");
}


//----------------------------------------------------------------------------
// Exit command.
//----------------------------------------------------------------------------

ts::CommandStatus ts::tsp::ControlServer::executeExit(const UString& command, Args& args)
{
    if (args.present(u"abort")) {
        // Immediate exit.
        std::exit(EXIT_FAILURE);
    }
    else {
        _log.info(u"exit requested by remote tspcontrol command");
        // Place all threads in "aborted" state so that each thread will see its
        // successor as aborted. Notify all threads that something happened.
        PluginExecutor* proc = _input;
        do {
            proc->setAbort();
        } while ((proc = proc->ringNext<PluginExecutor>()) != _input);
    }
    return CommandStatus::SUCCESS;
}


//----------------------------------------------------------------------------
// Set-log command.
//----------------------------------------------------------------------------

ts::CommandStatus ts::tsp::ControlServer::executeSetLog(const UString& command, Args& args)
{
    const int level = args.intValue(u"", Severity::Info);

    // Set log severity of the main logger.
    _log.setMaxSeverity(level);
    _log.log(level, u"set log level to %s", Severity::Enums().name(level));

    // Also set the log severity on each individual plugin.
    std::lock_guard<std::recursive_mutex> lock(_global_mutex);
    PluginExecutor* proc = _input;
    do {
        proc->plugin()->setMaxSeverity(level);
    } while ((proc = proc->ringNext<ts::tsp::PluginExecutor>()) != _input);

    return CommandStatus::SUCCESS;
}


//----------------------------------------------------------------------------
// List command.
//----------------------------------------------------------------------------

ts::CommandStatus ts::tsp::ControlServer::executeList(const UString& command, Args& args)
{
    if (args.verbose()) {
        args.info(u"");
        args.info(u"Executable: %s", ExecutableFile());
        args.info(u"");
    }

    listOnePlugin(0, u'I', _input, args);
    size_t index = 1;
    for (size_t i = 0; i < _plugins.size(); ++i) {
        listOnePlugin(index++, u'P', _plugins[i], args);
    }
    listOnePlugin(index, u'O', _output, args);

    if (args.verbose()) {
        args.info(u"");
    }
    return CommandStatus::SUCCESS;
}

void ts::tsp::ControlServer::listOnePlugin(size_t index, UChar type, PluginExecutor* plugin, Report& report)
{
    const bool verbose = report.verbose();
    const bool suspended = plugin->getSuspended();
    report.info(u"%2d: %s-%c %s",
                index,
                verbose && suspended ? u"(suspended) " : u"",
                type,
                verbose ? plugin->plugin()->commandLine() : plugin->pluginName());
}


//----------------------------------------------------------------------------
// Suspend/resume commands.
//----------------------------------------------------------------------------

ts::CommandStatus ts::tsp::ControlServer::executeSuspend(const UString& command, Args& args)
{
    return executeSuspendResume(true, args);
}

ts::CommandStatus ts::tsp::ControlServer::executeResume(const UString& command, Args& args)
{
    return executeSuspendResume(false, args);
}

ts::CommandStatus ts::tsp::ControlServer::executeSuspendResume(bool state, Args& args)
{
    const size_t index = args.intValue<size_t>(u"");
    if (index > 0 && index <= _plugins.size()) {
        _plugins[index-1]->setSuspended(state);
    }
    else if (index == _plugins.size() + 1) {
        _output->setSuspended(state);
    }
    else if (index == 0) {
        args.error(u"cannot suspend/resume the input plugin");
    }
    else {
        args.error(u"invalid plugin index %d, specify 1 to %d", index, _plugins.size() + 1);
    }
    return CommandStatus::SUCCESS;
}


//----------------------------------------------------------------------------
// Restart commands.
//----------------------------------------------------------------------------

ts::CommandStatus ts::tsp::ControlServer::executeRestart(const UString& command, Args& args)
{
    // Get all parameters. The first one is the plugin index. Others are plugin parameters.
    UStringVector params;
    args.getValues(params);
    size_t index = 0;
    if (params.empty() || !params[0].toInteger(index) || index > _plugins.size() + 1) {
        args.error(u"invalid plugin index");
        return CommandStatus::ERROR;
    }

    // Keep only plugin parameters.
    params.erase(params.begin());

    // Same we use new parameters?
    const bool same = args.present(u"same");
    if (same && !params.empty()) {
        args.error(u"do not specify new plugin options with --same");
        return CommandStatus::ERROR;
    }

    // Get the target plugin.
    PluginExecutor* plugin = nullptr;
    if (index == 0) {
        plugin = _input;
    }
    else if (index <= _plugins.size()) {
        plugin = _plugins[index-1];
    }
    else {
        plugin = _output;
    }

    // Restart the plugin.
    if (same) {
        plugin->restart(args);
    }
    else {
        plugin->restart(params, args);
    }
    return CommandStatus::SUCCESS;
}
