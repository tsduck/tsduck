//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstsswitchCommandListener.h"
#include "tstsswitchCore.h"
#include "tsRestServer.h"
#include "tsReportBuffer.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tsswitch::CommandListener::CommandListener(Core& core, const InputSwitcherArgs& opt, Report& log) :
    _log(log),
    _core(core),
    _opt(opt)
{
}

ts::tsswitch::CommandListener::~CommandListener()
{
    // Terminate the thread and wait for actual thread termination.
    close();
    waitForTermination();
}


//----------------------------------------------------------------------------
// Start/stop the command receiver.
//----------------------------------------------------------------------------

bool ts::tsswitch::CommandListener::open()
{
    // Setup the receiver, either a TLS/TCP server or a UDP socket.
    if (_opt.remote_control.use_tls) {
        // Initialize TCP server.
        // The server will accept and process one client at a time.
        // Therefore, be generous with the backlog.
        if (!_tls_server.open(_opt.remote_control.server_addr.generation(), _log) ||
            !_tls_server.reusePort(_opt.remote_control.reuse_port, _log) ||
            !_tls_server.bind(_opt.remote_control.server_addr, _log) ||
            !_tls_server.listen(16, _log))
        {
            _tls_server.close(NULLREP);
            return false;
        }
        // Do not request client certificate (this is the default anyway).
        _tls_client.setVerifyPeer(false);
    }
    else {
        // Initialize a UDP reception socket.
        UDPReceiverArgs sock_args;
        sock_args.setUnicast(_opt.remote_control.server_addr, _opt.remote_control.reuse_port, _opt.sock_buffer_size);
        _udp_server.setParameters(sock_args);
        if (!_udp_server.open(_log)) {
            return false;
        }
    }

    // Start the thread.
    return start();
}

void ts::tsswitch::CommandListener::close()
{
    // Close the receiver. This will force the server thread to terminate.
    _terminate = true;
    if (_opt.remote_control.use_tls) {
        _tls_client.close(NULLREP);
        _tls_server.close(NULLREP);
    }
    else {
        _udp_server.close(NULLREP);
    }
}


//----------------------------------------------------------------------------
// Invoked in the context of the server thread.
//----------------------------------------------------------------------------

void ts::tsswitch::CommandListener::main()
{
    _log.debug(u"remote control server thread started");

    char inbuf[1024];
    size_t insize = 0;
    IPSocketAddress sender;
    IPSocketAddress destination;

    // Get receive errors in a buffer since some errors are normal.
    ReportBuffer<ThreadSafety::None> error(_log.maxSeverity());

    // Process commands, either from the TLS/TCP server or UDP socket.
    if (_opt.remote_control.use_tls) {
        // Loop on incoming TLS/TCP clients.
        while (!_terminate) {
            IPSocketAddress client_addr;
            // Do not terminate on accept() failure, this may be a client which fails the TLS handshake.
            if (_tls_server.accept(_tls_client, client_addr, _log)) {
                // Process a request. In case of error, getRequest() closes the connection
                RestServer rest(_opt.remote_control, _log);
                if (rest.getRequest(_tls_client)) {
                    // The command is the path of the request ("GET /next" for instance).
                    const bool success = rest.path().starts_with('/') && execute(client_addr, rest.path().substr(1));
                    // Send the response and close the connection.
                    if (success) {
                        rest.sendResponse(_tls_client, 204, true); // 204 = No Content
                    }
                    else {
                        rest.setResponse(u"Invalid command\n");
                        rest.sendResponse(_tls_client, 400, true); // 400 = Bad Request
                    }
                }
            }
        }
    }
    else {
        // Loop on incoming UDP datagrams.
        while (!_terminate && _udp_server.receive(inbuf, sizeof(inbuf), insize, sender, destination, nullptr, error)) {

            // Filter out unauthorized remote systems.
            if (!_opt.remote_control.isAllowed(sender)) {
                _log.warning(u"rejected remote command from unauthorized host %s", sender);
                continue;
            }

            // We expect ASCII commands. Locate first non-ASCII character in message.
            size_t len = 0;
            while (len < insize && inbuf[len] >= 0x20 && inbuf[len] <= 0x7E) {
                len++;
            }

            // Execute command.
            execute(sender, UString::FromUTF8(inbuf, len).toLower().toTrimmed());
        }
    }

    // If termination was requested, receive error is not an error.
    if (!_terminate && !error.empty()) {
        _log.info(error.messages());
    }
    _log.debug(u"remote control server thread completed");
}

//----------------------------------------------------------------------------
// Execute a remote command.
//----------------------------------------------------------------------------

bool ts::tsswitch::CommandListener::execute(const IPSocketAddress& sender, const UString& cmd)
{
    _log.verbose(u"received command \"%s\" from %s", cmd, sender);

    // Process the command.
    size_t index = 0;
    if (cmd.toInteger(index)) {
        _core.setInput(index);
    }
    else if (cmd == u"next") {
        _core.nextInput();
    }
    else if (cmd.starts_with(u"prev")) {
        _core.previousInput();
    }
    else if (cmd == u"quit" || cmd == u"exit") {
        _core.stop(true);
    }
    else if (cmd == u"halt" || cmd == u"abort") {
        // Extremely rude way of exiting the process.
        static const char err[] = "\n\n*** Emergency abort requested\n\n";
        FatalError(err, sizeof(err) - 1);
    }
    else {
        _log.error(u"received invalid command \"%s\" from remote control at %s", cmd, sender);
        return false;
    }
    return true;
}
