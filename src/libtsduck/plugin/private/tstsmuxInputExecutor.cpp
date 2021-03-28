//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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

#include "tstsmuxInputExecutor.h"
#include "tsGuard.h"
#include "tsGuardCondition.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tsmux::InputExecutor::InputExecutor(const MuxerArgs& opt, const PluginEventHandlerRegistry& handlers, size_t index, Report& log) :
    // Input threads have a high priority to be always ready to load incoming packets in the buffer.
    PluginExecutor(opt, handlers, PluginType::INPUT, opt.inputs[index], ThreadAttributes().setPriority(ThreadAttributes::GetHighPriority()), log),
    _input(dynamic_cast<InputPlugin*>(PluginThread::plugin())),
    _pluginIndex(index),
    _packets(_opt.inBufferPackets),
    _metadata(opt.inBufferPackets)
{
    // Make sure that the input plugins display their index.
    setLogName(UString::Format(u"%s[%d]", {pluginName(), _pluginIndex}));
}

ts::tsmux::InputExecutor::~InputExecutor()
{
    waitForTermination();
}


//----------------------------------------------------------------------------
// Implementation of TSP.
//----------------------------------------------------------------------------

size_t ts::tsmux::InputExecutor::pluginIndex() const
{
    return _pluginIndex;
}


//----------------------------------------------------------------------------
// Terminate input.
//----------------------------------------------------------------------------

void ts::tsmux::InputExecutor::terminateInput()
{
    //@@@@@@@@@@@
}


//----------------------------------------------------------------------------
// Invoked in the context of the plugin thread.
//----------------------------------------------------------------------------

void ts::tsmux::InputExecutor::main()
{
    debug(u"input thread started");

    //@@@@@@@@@@@@@@

    debug(u"input thread terminated");
}
