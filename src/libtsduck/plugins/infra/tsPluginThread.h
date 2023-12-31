//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
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
#include "tsPlugin.h"
#include "tsPluginOptions.h"

namespace ts {
    //!
    //! Base class for threads executing a tsp plugin.
    //! The subclasses shall implement the TSP interface.
    //! @ingroup plugin
    //!
    class TSDUCKDLL PluginThread: public Thread, public TSP
    {
        TS_NOBUILD_NOCOPY(PluginThread);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Initial report object.
        //! The @a report object is used to forward messages which are sent to this
        //! PluginThread instance (PluginThread is a subclass of Report through TSP).
        //! @param [in] appName Application name, for help messages.
        //! @param [in] type Plugin type.
        //! @param [in] options Command line options for this plugin.
        //! @param [in] attributes Creation attributes for the thread executing this plugin.
        //!
        PluginThread(Report* report, const UString& appName, PluginType type, const PluginOptions& options, const ThreadAttributes& attributes);

        //!
        //! Destructor
        //!
        virtual ~PluginThread() override;

        //!
        //! Change the report object.
        //! @param [in] rep Address of new report instance.
        //!
        void setReport(Report* rep) { _report = rep; }

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
        void setLogName(const UString& name) { _logname = name; }

        // Implementation of TSP virtual methods.
        virtual UString pluginName() const override;
        virtual Plugin* plugin() const override;

    protected:
        // Inherited from Report (via TSP)
        virtual void writeLog(int severity, const UString& msg) override;

    private:
        Report*       _report;  // Common report interface for all plugins
        const UString _name;    // Plugin name.
        UString       _logname; // Plugin name as displayed in log messages.
        Plugin*       _shlib;   // Shared library API.
    };
}
