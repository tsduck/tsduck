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

namespace ts::tsp {
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
        //! @param [in] core Shared pointer to the transport stream processor internal core API.
        //! @param [in] tsp_options Command line options for tsp.
        //! @param [in] handlers Registry of event handlers.
        //! @param [in] attributes Creation attributes for the thread executing this plugin.
        //!
        ProcessorExecutor(const std::shared_ptr<tsp::Core>& core,
                          const TSProcessorArgs& tsp_options,
                          const PluginEventHandlerRegistry& handlers,
                          size_t plugin_index,
                          const ThreadAttributes& attributes);

        //!
        //! Destructor.
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
