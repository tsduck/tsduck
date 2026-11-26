//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Input switch (tsswitch) plugin executor thread.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tstspPluginThread.h"
#include "tsInputSwitcherArgs.h"
#include "tsPluginEventHandlerRegistry.h"

namespace ts::tsswitch {

    class Core;

    //!
    //! Execution context of a tsswitch plugin.
    //! @ingroup libtsduck plugin
    //!
    class PluginExecutor: public tsp::PluginThread
    {
        TS_NOBUILD_NOCOPY(PluginExecutor);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp_core Shared pointer to the transport stream processor internal core API.
        //! @param [in] switch_options Command line options for tsswitch.
        //! @param [in] handlers Registry of event handlers.
        //! @param [in] type Plugin type.
        //! @param [in] plugin_options Command line options for this plugin.
        //! @param [in] attributes Creation attributes for the thread executing this plugin.
        //! @param [in,out] switch_core Input switcher command core instance.
        //!
        PluginExecutor(const std::shared_ptr<tsp::Core>& tsp_core,
                       const InputSwitcherArgs& switch_options,
                       const PluginEventHandlerRegistry& handlers,
                       tsswitch::Core& switch_core,
                       PluginType type,
                       const PluginOptions& plugin_options,
                       const ThreadAttributes& attributes);

        //!
        //! Destructor.
        //!
        virtual ~PluginExecutor() override;

        // Implementation of TSP.
        virtual size_t pluginCount() const override;
        virtual void signalPluginEvent(uint32_t event_code, Object* plugin_data = nullptr) const override;

    protected:
        tsswitch::Core&                   _switch_core;     //!< Input switcher command core instance.
        const InputSwitcherArgs&          _switch_options;  //!< Command line options.
        const PluginEventHandlerRegistry& _handlers;        //!< Registry of event handlers.
    };
}
