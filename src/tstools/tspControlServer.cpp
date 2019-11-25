//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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

#include "tspControlServer.h"
#include "tspPluginExecutor.h"
#include "tsNullMutex.h"
#include "tsNullReport.h"
#include "tsReportBuffer.h"
#include "tsTelnetConnection.h"
#include "tsGuard.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tsp::ControlServer::ControlServer(Options& options, Report& log, Mutex& global_mutex, InputExecutor* input) :
    _is_open(false),
    _terminate(false),
    _options(options),
    _log(log, u"control commands: "),
    _reference(),
    _server(),
    _mutex(global_mutex),
    _input(input),
    _output(nullptr),
    _plugins()
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
        if (std::find(_options.control_sources.begin(), _options.control_sources.end(), source) == _options.control_sources.end()) {
            _log.warning(u"connection attempt from unauthorized source %s (ignored)", {source});
            conn.sendLine("error: client address is not authorized", _log);
        }
        else if (conn.setReceiveTimeout(_options.control_timeout, _log) && conn.receiveLine(line, nullptr, _log)) {
            _log.verbose(u"received from %s: %s", {source, line});
            executeCommand(line, conn);
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
// Execute one command.
//----------------------------------------------------------------------------

void ts::tsp::ControlServer::executeCommand(const UString& line, Report& log)
{
    ControlCommand cmd = ControlCommand(0);
    const Args* args = nullptr;

    // Analyze the command, return errors on the client connection.
    if (_reference.analyze(line, cmd, args, log) && args != nullptr) {
        // Dispatch the command.
        switch (cmd) {
            case CMD_EXIT: {
                executeExit(args, log);
                return;
            }
            case CMD_SETLOG: {
                executeSetLog(args, log);
                return;
            }
            case CMD_LIST: {
                executeList(args, log);
                return;
            }
            case CMD_SUSPEND: {
                executeSuspend(args, log);
                return;
            }
            case CMD_RESUME: {
                executeResume(args, log);
                return;
            }
            default: {
                break;
            }
        }
    }

    // At this point, the command is incorrect.
    log.error(u"invalid tsp control command: %s", {line});
}


//----------------------------------------------------------------------------
// Command handlers.
//----------------------------------------------------------------------------

void ts::tsp::ControlServer::executeExit(const Args* args, Report& log)
{
    if (args->present(u"abort")) {
        // Immediate exit.
        ::exit(EXIT_FAILURE);
    }
    else {
        // Send an end of stream to input plugin
        //@@@@@@
    }
}

void ts::tsp::ControlServer::executeSetLog(const Args* args, Report& log)
{
    const int level = args->intValue(u"", Severity::Info);
    _log.setMaxSeverity(args->intValue(u"", Severity::Info));
    _log.log(level, u"set log level to %s", {Severity::Enums.name(level)});
}

void ts::tsp::ControlServer::executeList(const Args* args, Report& log)
{
    //@@@@@@
}

void ts::tsp::ControlServer::executeSuspend(const Args* args, Report& log)
{
    //@@@@@@
}

void ts::tsp::ControlServer::executeResume(const Args* args, Report& log)
{
    //@@@@@@
}
