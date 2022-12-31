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

#include "tstsswitchCommandListener.h"
#include "tstsswitchCore.h"
#include "tsNullMutex.h"
#include "tsReportBuffer.h"
#include "tsAlgorithm.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tsswitch::CommandListener::CommandListener(Core& core, const InputSwitcherArgs& opt, Report& log) :
    _log(log),
    _core(core),
    _opt(opt),
    _sock(_log),
    _terminate(false)
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
    // Set command line parameters.
    _sock.setParameters(_opt.remoteServer, _opt.reusePort, _opt.sockBuffer);

    // Open the UDP receiver and start the thread.
    return _sock.open(_log) && start();
}

void ts::tsswitch::CommandListener::close()
{
    // Close the UDP receiver. This will force the server thread to terminate.
    _terminate = true;
    _sock.close(NULLREP);
}


//----------------------------------------------------------------------------
// Invoked in the context of the server thread.
//----------------------------------------------------------------------------

void ts::tsswitch::CommandListener::main()
{
    _log.debug(u"UDP server thread started");

    char inbuf[1024];
    size_t insize = 0;
    IPv4SocketAddress sender;
    IPv4SocketAddress destination;

    // Get receive errors in a buffer since some errors are normal.
    ReportBuffer<NullMutex> error(_log.maxSeverity());

    // Loop on incoming messages.
    while (_sock.receive(inbuf, sizeof(inbuf), insize, sender, destination, nullptr, error)) {

        // Filter out unauthorized remote systems.
        if (!_opt.allowedRemote.empty() && !Contains(_opt.allowedRemote, sender)) {
            _log.warning(u"rejected remote command from unauthorized host %s", {sender});
            continue;
        }

        // We expect ASCII commands. Locate first non-ASCII character in message.
        size_t len = 0;
        while (len < insize && inbuf[len] >= 0x20 && inbuf[len] <= 0x7E) {
            len++;
        }

        // Extract trimmed lowercase ASCII command.
        UString cmd(UString::FromUTF8(inbuf, len));
        cmd.toLower();
        cmd.trim();
        _log.verbose(u"received command \"%s\", from %s (%d bytes)", {cmd, sender, insize});

        // Process the command (case insensitive).
        size_t index = 0;
        if (cmd.toInteger(index)) {
            _core.setInput(index);
        }
        else if (cmd == u"next") {
            _core.nextInput();
        }
        else if (cmd.startWith(u"prev")) {
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
            _log.error(u"received invalid command \"%s\" from remote control at %s", {cmd, sender});
        }
    }

    // If termination was requested, receive error is not an error.
    if (!_terminate && !error.emptyMessages()) {
        _log.info(error.getMessages());
    }
    _log.debug(u"UDP server thread completed");
}
