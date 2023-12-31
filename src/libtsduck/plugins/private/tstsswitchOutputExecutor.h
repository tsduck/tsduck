//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
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

namespace ts {
    namespace tsswitch {
        //!
        //! Execution context of a tsswitch output plugin.
        //! @ingroup plugin
        //!
        class OutputExecutor : public PluginExecutor
        {
            TS_NOBUILD_NOCOPY(OutputExecutor);
        public:
            //!
            //! Constructor.
            //! @param [in] opt Command line options.
            //! @param [in] handlers Registry of event handlers.
            //! @param [in,out] core Command core instance.
            //! @param [in,out] log Log report.
            //!
            OutputExecutor(const InputSwitcherArgs& opt,
                           const PluginEventHandlerRegistry& handlers,
                           Core& core,
                           Report& log);

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
            OutputPlugin* _output;     // Plugin API.
            volatile bool _terminate;  // Termination request.

            // Implementation of Thread.
            virtual void main() override;
        };
    }
}
