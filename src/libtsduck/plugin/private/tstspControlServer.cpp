//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tstspControlServer.h"
#include "tstspPluginExecutor.h"
#include "tsNullMutex.h"
#include "tsNullReport.h"
#include "tsReportBuffer.h"
#include "tsTelnetConnection.h"
#include "tsGuard.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tsp::ControlServer::ControlServer(TSProcessorArgs& options, Report& log, Mutex& global_mutex, InputExecutor* input) :
    _is_open(false),
    _terminate(false),
    _options(options),
    _log(log, u"control commands: "),
    _reference(),
    _server(),
    _mutex(global_mutex),
    _input(input),
    _output(nullptr),
    _plugins(),
    _handlers{{TSPControlCommand::CMD_EXIT,    &ControlServer::executeExit},
              {TSPControlCommand::CMD_SETLOG,  &ControlServer::executeSetLog},
              {TSPControlCommand::CMD_LIST,    &ControlServer::executeList},
              {TSPControlCommand::CMD_SUSPEND, &ControlServer::executeSuspend},
              {TSPControlCommand::CMD_RESUME,  &ControlServer::executeResume},
              {TSPControlCommand::CMD_RESTART, &ControlServer::executeRestart}}
{
    // Locate output plugin, count packet processor plugins.
    if (_input != nullptr) {
        Guard lock(_mutex);

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
    _log.debug(u"found %d packet processor plugins", {_plugins.size()});
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
        const SocketAddress addr(_options.control_local, _options.control_port);
        if (!_server.open(_log) ||
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
    ReportBuffer<NullMutex> error(_log.maxSeverity());

    // Client address and connection.
    SocketAddress source;
    TelnetConnection conn;
    UString line;

    // Loop on incoming connections.
    // Since the commands are expected to be short, treat only one at a time.
    while (_server.accept(conn, source, error)) {

        // Filter allowed sources.
        // Set receive timeout on the connection and read one line.
        if (std::find(_options.control_sources.begin(), _options.control_sources.end(), source) == _options.control_sources.end()) {
            _log.warning(u"connection attempt from unauthorized source %s (ignored)", {source});
            conn.sendLine("error: client address is not authorized", _log);
        }
        else if (conn.setReceiveTimeout(_options.control_timeout, _log) && conn.receiveLine(line, nullptr, _log)) {
            _log.verbose(u"received from %s: %s", {source, line});

            // Reset the severity of the connection before analysing the line.
            // A previous analysis may have used --verbose or --debug.
            conn.setMaxSeverity(Severity::Info);

            // Analyze the command, return errors on the client connection.
            TSPControlCommand::ControlCommand cmd = TSPControlCommand::CMD_NONE;
            const Args* args = nullptr;
            CommandHandler handler = nullptr;
            if (_reference.analyze(line, cmd, args, conn)) {
                const auto it = _handlers.find(cmd);
                if (it != _handlers.end()) {
                    handler = it->second;
                }
            }

            // Execute the handler for this command or return an error message.
            if (handler != nullptr && args != nullptr) {
                // Execute the command.
                (this->*handler)(args, conn);
            }
            else {
                conn.error(u"invalid tsp control command: %s", {line});
            }
        }

        conn.closeWriter(_log);
        conn.close(_log);
    }

    // If termination was requested, receive error is not an error.
    if (!_terminate && !error.emptyMessages()) {
        _log.error(error.getMessages());
    }
    _log.debug(u"control command thread completed");
}


//----------------------------------------------------------------------------
// Exit command.
//----------------------------------------------------------------------------

void ts::tsp::ControlServer::executeExit(const Args* args, Report& response)
{
    if (args->present(u"abort")) {
        // Immediate exit.
        ::exit(EXIT_FAILURE);
    }
    else {
        _log.info(u"exit requested by remote tcpcontrol");
        // Place all threads in "aborted" state so that each thread will see its
        // successor as aborted. Notify all threads that something happened.
        PluginExecutor* proc = _input;
        do {
            proc->setAbort();
        } while ((proc = proc->ringNext<PluginExecutor>()) != _input);
    }
}


//----------------------------------------------------------------------------
// Set-log command.
//----------------------------------------------------------------------------

void ts::tsp::ControlServer::executeSetLog(const Args* args, Report& response)
{
    const int level = args->intValue(u"", Severity::Info);

    // Set log severity of the main logger.
    _log.setMaxSeverity(level);
    _log.log(level, u"set log level to %s", {Severity::Enums.name(level)});

    // Also set the log severity on each individual plugin.
    Guard lock(_mutex);
    PluginExecutor* proc = _input;
    do {
        proc->setMaxSeverity(level);
    } while ((proc = proc->ringNext<ts::tsp::PluginExecutor>()) != _input);
}


//----------------------------------------------------------------------------
// List command.
//----------------------------------------------------------------------------

void ts::tsp::ControlServer::executeList(const Args* args, Report& response)
{
    if (response.verbose()) {
        response.info(u"");
        response.info(u"Executable: %s", {ExecutableFile()});
        response.info(u"");
    }

    listOnePlugin(0, u'I', _input, response);
    size_t index = 1;
    for (size_t i = 0; i < _plugins.size(); ++i) {
        listOnePlugin(index++, u'P', _plugins[i], response);
    }
    listOnePlugin(index, u'O', _output, response);

    if (response.verbose()) {
        response.info(u"");
    }
}

void ts::tsp::ControlServer::listOnePlugin(size_t index, UChar type, PluginExecutor* plugin, Report& response)
{
    const bool verbose = response.verbose();
    const bool suspended = plugin->getSuspended();
    response.info(u"%2d: %s-%c %s", {
                  index,
                  verbose && suspended ? u"(suspended) " : u"",
                  type,
                  verbose ? plugin->plugin()->commandLine() : plugin->pluginName() });
}


//----------------------------------------------------------------------------
// Suspend/resume commands.
//----------------------------------------------------------------------------

void ts::tsp::ControlServer::executeSuspend(const Args* args, Report& response)
{
    executeSuspendResume(true, args, response);
}

void ts::tsp::ControlServer::executeResume(const Args* args, Report& response)
{
    executeSuspendResume(false, args, response);
}

void ts::tsp::ControlServer::executeSuspendResume(bool state, const Args* args, Report& response)
{
    const size_t index = args->intValue<size_t>(u"");
    if (index > 0 && index <= _plugins.size()) {
        _plugins[index-1]->setSuspended(state);
    }
    else if (index == _plugins.size() + 1) {
        _output->setSuspended(state);
    }
    else if (index == 0) {
        response.error(u"cannot suspend/resume the input plugin");
    }
    else {
        response.error(u"invalid plugin index %d, specify 1 to %d", { index, _plugins.size() + 1 });
    }
}


//----------------------------------------------------------------------------
// Restart commands.
//----------------------------------------------------------------------------

void ts::tsp::ControlServer::executeRestart(const Args* args, Report& response)
{
    // Get all parameters. The first one is the plugin index. Others are plugin parameters.
    UStringVector params;
    args->getValues(params);
    size_t index = 0;
    if (params.empty() || !params[0].toInteger(index) || index > _plugins.size() + 1) {
        response.error(u"invalid plugin index");
        return;
    }

    // Keep only plugin parameters.
    params.erase(params.begin());

    // Same we use new parameters?
    const bool same = args->present(u"same");
    if (same && !params.empty()) {
        response.error(u"do not specify new plugin options with --same");
        return;
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
        plugin->restart(response);
    }
    else {
        plugin->restart(params, response);
    }
}
