//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for threads executing a tsp plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsThread.h"
#include "tsTSP.h"
#include "tsPlugin.h"
#include "tsPluginOptions.h"

namespace ts::tsp {
    //!
    //! Base class for threads executing a tsp plugin.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL PluginThread: public Thread, public TSP
    {
        TS_NOBUILD_NOCOPY(PluginThread);
    public:
        //!
        //! Constructor.
        //! @param [in] core Shared pointer to the transport stream processor internal core API.
        //! @param [in] type Plugin type.
        //! @param [in] plugin_options Command line options for this plugin.
        //! @param [in] attributes Creation attributes for the thread executing this plugin.
        //!
        PluginThread(const std::shared_ptr<tsp::Core>& core, PluginType type, const PluginOptions& plugin_options, const ThreadAttributes& attributes);

        //!
        //! Destructor.
        //!
        virtual ~PluginThread() override;

        //!
        //! Plugin stack size overhead.
        //! Each plugin defines its own usage of the stack. The PluginThread
        //! class and its subclasses have their own additional stack usage.
        //!
        static constexpr size_t STACK_SIZE_OVERHEAD = 32 * 1024; // 32 kB

        //!
        //! Set the plugin name as displayed in log messages.
        //! By default, used the real plugin name.
        //! @param [in] name The name to use in log messages.
        //! When empty, revert to the real plugin name.
        //!
        void setLogName(const UString& name);

        // Implementation of TSP virtual methods.
        virtual UString pluginName() const override;
        virtual Plugin* plugin() const override;

    private:
        UString _plugin_name;  // Plugin name.
        Plugin* _plugin;       // Plugin instance.
    };
}
