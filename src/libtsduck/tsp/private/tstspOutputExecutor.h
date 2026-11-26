//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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

namespace ts::tsp {
    //!
    //! Execution context of a tsp output plugin.
    //! This class is internal to the TSDuck library and cannot be called by applications.
    //! @ingroup libtsduck plugin
    //!
    class OutputExecutor: public PluginExecutor
    {
        TS_NOBUILD_NOCOPY(OutputExecutor);
    public:
        //!
        //! Constructor.
        //! @param [in] core Shared pointer to the transport stream processor internal core API.
        //! @param [in] tsp_options Command line options for tsp.
        //! @param [in] handlers Registry of event handlers.
        //! @param [in] plugin_options Command line options for this plugin.
        //! @param [in] attributes Creation attributes for the thread executing this plugin.
        //!
        OutputExecutor(const std::shared_ptr<tsp::Core>& core,
                       const TSProcessorArgs& tsp_options,
                       const PluginEventHandlerRegistry& handlers,
                       const PluginOptions& plugin_options,
                       const ThreadAttributes& attributes);

        //!
        //! Destructor.
        //!
        virtual ~OutputExecutor() override;

        // Overridden methods.
        virtual size_t pluginIndex() const override;

    private:
        OutputPlugin* _output = nullptr;

        // Inherited from Thread
        virtual void main() override;
    };
}
