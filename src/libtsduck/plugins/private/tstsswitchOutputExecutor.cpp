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

#include "tstsswitchOutputExecutor.h"
#include "tstsswitchCore.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tsswitch::OutputExecutor::OutputExecutor(const InputSwitcherArgs& opt,
                                             const PluginEventHandlerRegistry& handlers,
                                             Core& core,
                                             Report& log) :

    PluginExecutor(opt, handlers, PluginType::OUTPUT, opt.output, ThreadAttributes(), core, log),
    _output(dynamic_cast<OutputPlugin*>(plugin())),
    _terminate(false)
{
}

ts::tsswitch::OutputExecutor::~OutputExecutor()
{
    waitForTermination();
}


//----------------------------------------------------------------------------
// Implementation of TSP.
//----------------------------------------------------------------------------

size_t ts::tsswitch::OutputExecutor::pluginIndex() const
{
    // The output plugin comes last.
    return _opt.inputs.size();
}


//----------------------------------------------------------------------------
// Invoked in the context of the output plugin thread.
//----------------------------------------------------------------------------

void ts::tsswitch::OutputExecutor::main()
{
    debug(u"output thread started");

    size_t pluginIndex = 0;
    TSPacket* first = nullptr;
    TSPacketMetadata* metadata = nullptr;
    size_t count = 0;

    // Loop until there are packets to output.
    while (!_terminate && _core.getOutputArea(pluginIndex, first, metadata, count)) {
        log(2, u"got %d packets from plugin %d, terminate: %s", {count, pluginIndex, _terminate});
        if (!_terminate && count > 0) {

            // Output the packets.
            const bool success = _output->send(first, metadata, count);

            // Signal to the input plugin that the buffer can be reused..
            _core.outputSent(pluginIndex, count);

            // Abort the whole process in case of output error.
            if (success) {
                addPluginPackets(count);
            }
            else {
                debug(u"stopping output plugin");
                _core.stop(false);
                _terminate = true;
            }
        }
    }

    // Stop the plugin.
    _output->stop();
    debug(u"output thread terminated");
}
