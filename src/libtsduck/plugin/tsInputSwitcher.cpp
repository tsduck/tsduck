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

#include "tsInputSwitcher.h"
#include "tsSystemMonitor.h"
#include "tstsswitchCore.h"
#include "tstsswitchCommandListener.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::InputSwitcher::InputSwitcher(const InputSwitcherArgs& args, Report& report) :
    PluginEventHandlerRegistry(),
    _success(false)
{
    // Clear errors on the report, used to check further initialisation errors.
    report.resetErrors();

    // Create the tsswitch core instance.
    tsswitch::Core core(args, *this, report);
    if (report.gotErrors()) {
        return; // error
    }

    // Create a monitoring thread if required.
    ts::SystemMonitor monitor(&report);
    if (args.monitor) {
        monitor.start();
    }

    // If a remote control is specified, start a UDP listener thread.
    tsswitch::CommandListener remoteControl(core, args, report);
    if (args.remoteServer.hasPort() && !remoteControl.open()) {
        return; // error
    }

    // Start the processing.
    if (!core.start()) {
        return; // error
    }

    // Wait for completion.
    core.waitForTermination();

    // Now, we have a successful completion.
    _success = true;
}
