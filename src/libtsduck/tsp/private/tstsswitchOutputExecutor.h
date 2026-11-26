//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Input switch (tsswitch) output plugin executor thread.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstsswitchPluginExecutor.h"
#include "tsInputSwitcherArgs.h"
#include "tsOutputPlugin.h"

namespace ts::tsswitch {
    //!
    //! Execution context of a tsswitch output plugin.
    //! @ingroup libtsduck plugin
    //!
    class OutputExecutor: public PluginExecutor
    {
        TS_NOBUILD_NOCOPY(OutputExecutor);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp_core Shared pointer to the transport stream processor internal core API.
        //! @param [in] switch_options Command line options for tsswitch.
        //! @param [in] handlers Registry of event handlers.
        //! @param [in,out] switch_core Input switcher command core instance.
        //!
        OutputExecutor(const std::shared_ptr<tsp::Core>& tsp_core,
                       const InputSwitcherArgs& switch_options,
                       const PluginEventHandlerRegistry& handlers,
                       tsswitch::Core& switch_core);

        //!
        //! Virtual destructor.
        //!
        virtual ~OutputExecutor() override;

        //!
        //! Request the termination of the thread.
        //! Actual termination will occur after completion of the current output operation.
        //!
        void terminateOutput() { _terminate = true; }

        // Implementation of TSP.
        virtual size_t pluginIndex() const override;

    private:
        OutputPlugin*    _output;     // Plugin API.
        std::atomic_bool _terminate;  // Termination request.

        // Implementation of Thread.
        virtual void main() override;
    };
}
