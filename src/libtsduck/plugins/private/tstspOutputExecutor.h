//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream processor: Execution context of an output plugin
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstspPluginExecutor.h"
#include "tsOutputPlugin.h"

namespace ts {
    namespace tsp {
        //!
        //! Execution context of a tsp output plugin.
        //! This class is internal to the TSDuck library and cannot be called by applications.
        //! @ingroup plugin
        //!
        class OutputExecutor: public PluginExecutor
        {
            TS_NOBUILD_NOCOPY(OutputExecutor);
        public:
            //!
            //! Constructor.
            //! @param [in] options Command line options for tsp.
            //! @param [in] handlers Registry of event handlers.
            //! @param [in] pl_options Command line options for this plugin.
            //! @param [in] attributes Creation attributes for the thread executing this plugin.
            //! @param [in,out] global_mutex Global mutex to synchronize access to the packet buffer.
            //! @param [in,out] report Where to report logs.
            //!
            OutputExecutor(const TSProcessorArgs& options,
                           const PluginEventHandlerRegistry& handlers,
                           const PluginOptions& pl_options,
                           const ThreadAttributes& attributes,
                           Mutex& global_mutex,
                           Report* report);

            //!
            //! Virtual destructor.
            //!
            virtual ~OutputExecutor() override;

            // Overridden methods.
            virtual size_t pluginIndex() const override;

        private:
            OutputPlugin* _output;

            // Inherited from Thread
            virtual void main() override;
        };
    }
}
