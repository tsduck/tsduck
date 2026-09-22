//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream processor: Execution context of a packet processor plugin
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstspPluginExecutor.h"
#include "tsProcessorPlugin.h"

namespace ts {
    namespace tsp {
        //!
        //! Execution context of a tsp packet processor plugin.
        //! This class is internal to the TSDuck library and cannot be called by applications.
        //! @ingroup libtsduck plugin
        //!
        class ProcessorExecutor: public PluginExecutor
        {
            TS_NOBUILD_NOCOPY(ProcessorExecutor);
        public:
            //!
            //! Constructor.
            //! @param [in] options Command line options for tsp.
            //! @param [in] handlers Registry of event handlers.
            //! @param [in] plugin_index Index of command line options for this plugin in @a options.
            //! @param [in] attributes Creation attributes for the thread executing this plugin.
            //! @param [in,out] global_mutex Global mutex to synchronize access to the packet buffer.
            //! @param [in,out] report Where to report logs.
            //!
            ProcessorExecutor(const TSProcessorArgs& options,
                              const PluginEventHandlerRegistry& handlers,
                              size_t plugin_index,
                              const ThreadAttributes& attributes,
                              std::recursive_mutex& global_mutex,
                              Report* report);

            //!
            //! Virtual destructor.
            //!
            virtual ~ProcessorExecutor() override;

            // Overridden methods.
            virtual size_t pluginIndex() const override;

        private:
            ProcessorPlugin* _processor = nullptr;
            const size_t _plugin_index;

            // Inherited from Thread
            virtual void main() override;

            // Process packets one by one or using packet windows.
            void processIndividualPackets();
            void processPacketWindows(size_t window_size);
        };
    }
}
