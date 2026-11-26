//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tstsswitchOutputExecutor.h"
#include "tstsswitchCore.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::tsswitch::OutputExecutor::OutputExecutor(const std::shared_ptr<tsp::Core>& tsp_core,
                                             const InputSwitcherArgs& switch_options,
                                             const PluginEventHandlerRegistry& handlers,
                                             tsswitch::Core& switch_core) :

    PluginExecutor(tsp_core, switch_options, handlers, switch_core, PluginType::OUTPUT, switch_options.output, ThreadAttributes()),
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
    return _switch_options.inputs.size();
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
    while (!_terminate && _switch_core.getOutputArea(pluginIndex, first, metadata, count)) {
        log(2, u"got %d packets from plugin %d, terminate: %s", count, pluginIndex, _terminate);
        if (!_terminate && count > 0) {

            // Output the packets.
            const bool success = _output->send(first, metadata, count);

            // Signal to the input plugin that the buffer can be reused..
            _switch_core.outputSent(pluginIndex, count);

            // Abort the whole process in case of output error.
            if (success) {
                addPluginPackets(count);
            }
            else {
                debug(u"stopping output plugin");
                _switch_core.stop(false);
                _terminate = true;
            }
        }
    }

    // Stop the plugin.
    _output->stop();
    debug(u"output thread terminated");
}
