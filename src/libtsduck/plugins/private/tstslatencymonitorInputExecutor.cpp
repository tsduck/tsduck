//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Amos Cheung
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

#include "tstslatencymonitorInputExecutor.h"
#include "tsLatencyMonitor.h"

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
constexpr size_t ts::tslatencymonitor::InputExecutor::BUFFERED_PACKETS;
#endif


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tslatencymonitor::InputExecutor::InputExecutor(const LatencyMonitorArgs& opt,
                                                   size_t index,
                                                   LatencyMonitor& monitor,
                                                   Report& log) :

    // Input threads have a high priority to be always ready to load incoming packets in the buffer.
    PluginThread(&log, opt.appName, PluginType::INPUT, opt.inputs[index], ThreadAttributes().setPriority(ThreadAttributes::GetHighPriority())),
    _monitor(monitor),
    _input(dynamic_cast<InputPlugin*>(PluginThread::plugin())),
    _pluginIndex(index),
    _pluginCount(opt.inputs.size()),
    _buffer(BUFFERED_PACKETS),
    _metadata(BUFFERED_PACKETS)
{
    // Make sure that the input plugins display their index.
    setLogName(UString::Format(u"%s[%d]", {pluginName(), _pluginIndex}));
}

ts::tslatencymonitor::InputExecutor::~InputExecutor()
{
    waitForTermination();
}


//----------------------------------------------------------------------------
// Implementation of TSP. We do not use "joint termination" in tslatencymonitor.
//----------------------------------------------------------------------------

void ts::tslatencymonitor::InputExecutor::useJointTermination(bool)
{
}

void ts::tslatencymonitor::InputExecutor::jointTerminate()
{
}

bool ts::tslatencymonitor::InputExecutor::useJointTermination() const
{
    return false;
}

bool ts::tslatencymonitor::InputExecutor::thisJointTerminated() const
{
    return false;
}

size_t ts::tslatencymonitor::InputExecutor::pluginCount() const
{
    return _pluginCount;
}

void ts::tslatencymonitor::InputExecutor::signalPluginEvent(uint32_t event_code, Object* plugin_data) const
{
}

size_t ts::tslatencymonitor::InputExecutor::pluginIndex() const
{
    return _pluginIndex;
}


//----------------------------------------------------------------------------
// Invoked in the context of the plugin thread.
//----------------------------------------------------------------------------

void ts::tslatencymonitor::InputExecutor::main()
{
    debug(u"input thread started");

    // Main loop. Each iteration is a complete input session.
    for (;;) {
        // Here, we need to start an input session.
        debug(u"starting input plugin");
        const bool started = _input->start();
        debug(u"input plugin started, status: %s", {started});

        // Loop on incoming packets.
        for (;;) {
            // Input area (first packet index and packet count).
            size_t count;

            // Receive packets.
            if ((count = _input->receive(&_buffer[0], &_metadata[0], BUFFERED_PACKETS)) == 0) {
                // End of input.
                debug(u"received end of input from plugin");
                break;
            }

            // Pass packet to monitor for analyzing
            _monitor.processPacket(_buffer, _metadata, count, _pluginIndex);
        }
    }
}
