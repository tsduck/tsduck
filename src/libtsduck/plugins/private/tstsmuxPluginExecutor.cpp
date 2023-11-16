//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstsmuxPluginExecutor.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tsmux::PluginExecutor::PluginExecutor(const MuxerArgs& opt,
                                          const PluginEventHandlerRegistry& handlers,
                                          PluginType type,
                                          const PluginOptions& pl_options,
                                          const ThreadAttributes& attributes,
                                          Report& log) :

    PluginThread(&log, opt.appName, type, pl_options, attributes),
    _opt(opt),
    _buffer_size(type == PluginType::INPUT ? _opt.inBufferPackets : _opt.outBufferPackets),
    _handlers(handlers)
{
    // Preset common default options.
    if (plugin() != nullptr) {
        plugin()->resetContext(_opt.duckArgs);
    }
}

ts::tsmux::PluginExecutor::~PluginExecutor()
{
    // Wait for thread termination.
    waitForTermination();
}


//----------------------------------------------------------------------------
// Implementation of TSP. We do not use "joint termination" in tsmux.
//----------------------------------------------------------------------------

void ts::tsmux::PluginExecutor::useJointTermination(bool)
{
}

void ts::tsmux::PluginExecutor::jointTerminate()
{
}

bool ts::tsmux::PluginExecutor::useJointTermination() const
{
    return false;
}

bool ts::tsmux::PluginExecutor::thisJointTerminated() const
{
    return false;
}

size_t ts::tsmux::PluginExecutor::pluginCount() const
{
    // All inputs plus one output.
    return _opt.inputs.size() + 1;
}


//----------------------------------------------------------------------------
// Signal a plugin event.
//----------------------------------------------------------------------------

void ts::tsmux::PluginExecutor::signalPluginEvent(uint32_t event_code, Object* plugin_data) const
{
    const PluginEventContext ctx(event_code, pluginName(), pluginIndex(), pluginCount(), plugin(), plugin_data, bitrate(), pluginPackets(), totalPacketsInThread());
    _handlers.callEventHandlers(ctx);
}


//----------------------------------------------------------------------------
// Request the termination of the thread.
//----------------------------------------------------------------------------

void ts::tsmux::PluginExecutor::terminate()
{
    // Locked the mutex on behalf of the two conditions.
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _terminate = true;
    _got_packets.notify_all();
    _got_freespace.notify_all();
}
