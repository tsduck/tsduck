//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstsswitchPluginExecutor.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tsswitch::PluginExecutor::PluginExecutor(const std::shared_ptr<tsp::Core>& tsp_core,
                                             const InputSwitcherArgs& switch_options,
                                             const PluginEventHandlerRegistry& handlers,
                                             tsswitch::Core& switch_core,
                                             PluginType type,
                                             const PluginOptions& plugin_options,
                                             const ThreadAttributes& attributes) :

    tsp::PluginThread(tsp_core, type, plugin_options, attributes),
    _switch_core(switch_core),
    _switch_options(switch_options),
    _handlers(handlers)
{
}

ts::tsswitch::PluginExecutor::~PluginExecutor()
{
    // Wait for thread termination.
    waitForTermination();
}


//----------------------------------------------------------------------------
// Implementation of TSP.
//----------------------------------------------------------------------------

size_t ts::tsswitch::PluginExecutor::pluginCount() const
{
    // All inputs plus one output.
    return _switch_options.inputs.size() + 1;
}


//----------------------------------------------------------------------------
// Signal a plugin event.
//----------------------------------------------------------------------------

void ts::tsswitch::PluginExecutor::signalPluginEvent(uint32_t event_code, Object* plugin_data) const
{
    const PluginEventContext ctx(event_code, pluginName(), pluginIndex(), pluginCount(), plugin(), plugin_data, bitrate(), pluginPackets(), totalPacketsInThread());
    _handlers.callEventHandlers(ctx);
}
