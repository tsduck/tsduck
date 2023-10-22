//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Input switch (tsswitch) plugin executor thread.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPluginThread.h"
#include "tsInputSwitcherArgs.h"
#include "tsPluginEventHandlerRegistry.h"

namespace ts {
    namespace tsswitch {

        class Core;

        //!
        //! Execution context of a tsswitch plugin.
        //! @ingroup plugin
        //!
        class PluginExecutor : public PluginThread
        {
            TS_NOBUILD_NOCOPY(PluginExecutor);
        public:
            //!
            //! Constructor.
            //! @param [in] opt Command line options.
            //! @param [in] handlers Registry of event handlers.
            //! @param [in] type Plugin type.
            //! @param [in] pl_options Command line options for this plugin.
            //! @param [in] attributes Creation attributes for the thread executing this plugin.
            //! @param [in,out] core Command core instance.
            //! @param [in,out] log Log report.
            //!
            PluginExecutor(const InputSwitcherArgs& opt,
                           const PluginEventHandlerRegistry& handlers,
                           PluginType type,
                           const PluginOptions& pl_options,
                           const ThreadAttributes& attributes,
                           Core& core,
                           Report& log);

            //!
            //! Destructor.
            //!
            virtual ~PluginExecutor() override;

            // Implementation of TSP. We do not use "joint termination" in tsswitch.
            virtual void useJointTermination(bool) override;
            virtual void jointTerminate() override;
            virtual bool useJointTermination() const override;
            virtual bool thisJointTerminated() const override;
            virtual size_t pluginCount() const override;
            virtual void signalPluginEvent(uint32_t event_code, Object* plugin_data = nullptr) const override;

        protected:
            const InputSwitcherArgs& _opt;   //!< Command line options.
            Core&                    _core;  //!< Application core.

        private:
            const PluginEventHandlerRegistry& _handlers;  //!< Registry of event handlers.
        };
    }
}
