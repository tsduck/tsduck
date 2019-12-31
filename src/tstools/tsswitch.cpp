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
//
//  TS input switch based on input plugins.
//
//  Implementation notes:
//
//  The class Core implements the core function of tsswitch. It is used
//  by all other classes to get their instructions and report their status.
//
//  Each instance of the class InputExecutor implements a thread running one
//  input plugin.
//
//  The class OutputExecutor implements the thread running the single output
//  plugin. When started, it simply waits for packets from the current input
//  plugin and outputs them. The output threads stops when instructed by the
//  Switch object or in case of output error. In case of error, the output
//  threads sends a global stop command to the Switch object.
//
//  If the option --remote is used, an instance of the class CommandListener
//  starts a thread which listens to UDP commands. The received commands are
//  sent to the Switch object.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsswitchCore.h"
#include "tsswitchOptions.h"
#include "tsswitchInputExecutor.h"
#include "tsswitchOutputExecutor.h"
#include "tsswitchCommandListener.h"
#include "tsPluginRepository.h"
#include "tsSystemMonitor.h"
#include "tsAsyncReport.h"
#include "tsCerrReport.h"
TSDUCK_SOURCE;
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
//  Program main code.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Get command line options. Exit in case of errors.
    ts::tsswitch::Options opt(argc, argv);

    // Make sure that standard error displays the same level of messages as set in options.
    CERR.setMaxSeverity(opt.maxSeverity());

    // Create and start an asynchronous log (separate thread).
    ts::AsyncReport log(opt.maxSeverity(), opt.logTimeStamp, opt.logMaxBuffer, opt.logSynchronous);

    // Create the tsswitch core instance.
    ts::tsswitch::Core core(opt, log);
    opt.exitOnError();

    // Get the repository of plugins.
    ts::PluginRepository* plugins = ts::PluginRepository::Instance();
    ts::CheckNonNull(plugins);

    // If plugins were statically linked, disallow the dynamic loading of plugins.
#if defined(TSDUCK_STATIC_PLUGINS)
    plugins->setSharedLibraryAllowed(false);
#endif

    // Create a monitoring thread if required.
    ts::SystemMonitor monitor(&log);
    if (opt.monitor) {
        monitor.start();
    }

    // If a remote control is specified, start a UDP listener thread.
    ts::tsswitch::CommandListener remoteControl(core, opt, log);
    if (opt.remoteServer.hasPort() && !remoteControl.open()) {
        return EXIT_FAILURE;
    }

    // Start the processing.
    if (!core.start()) {
        return EXIT_FAILURE;
    }

    // Wait for completion.
    core.waitForTermination();
    return EXIT_SUCCESS;
}
