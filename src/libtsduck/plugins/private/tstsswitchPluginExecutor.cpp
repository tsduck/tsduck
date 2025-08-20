//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstsswitchPluginExecutor.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tsswitch::PluginExecutor::PluginExecutor(const InputSwitcherArgs& opt,
                                             const PluginEventHandlerRegistry& handlers,
                                             PluginType type,
                                             const PluginOptions& pl_options,
                                             const ThreadAttributes& attributes,
                                             Core& core,
                                             Report& log) :

    PluginThread(&log, opt.app_name, type, pl_options, attributes),
    _opt(opt),
    _core(core),
    _handlers(handlers)
{
}

ts::tsswitch::PluginExecutor::~PluginExecutor()
{
    // Wait for thread termination.
    waitForTermination();
}


//----------------------------------------------------------------------------
// Implementation of TSP. We do not use "joint termination" in tsswitch.
//----------------------------------------------------------------------------

void ts::tsswitch::PluginExecutor::useJointTermination(bool)
{
}

void ts::tsswitch::PluginExecutor::jointTerminate()
{
}

bool ts::tsswitch::PluginExecutor::useJointTermination() const
{
    return false;
}

bool ts::tsswitch::PluginExecutor::thisJointTerminated() const
{
    return false;
}

size_t ts::tsswitch::PluginExecutor::pluginCount() const
{
    // All inputs plus one output.
    return _opt.inputs.size() + 1;
}


//----------------------------------------------------------------------------
// Signal a plugin event.
//----------------------------------------------------------------------------

void ts::tsswitch::PluginExecutor::signalPluginEvent(uint32_t event_code, Object* plugin_data) const
{
    const PluginEventContext ctx(event_code, pluginName(), pluginIndex(), pluginCount(), plugin(), plugin_data, bitrate(), pluginPackets(), totalPacketsInThread());
    _handlers.callEventHandlers(ctx);
}
