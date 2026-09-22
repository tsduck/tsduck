//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream processor: Implementation of "Joint Termination"
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSProcessorArgs.h"
#include "tsPluginThread.h"

namespace ts {
    namespace tsp {
        //!
        //! Implementation of "Joint Termination" in the Transport stream processor.
        //! This is a subclass of ts::TSP and a superclass of all plugin executors.
        //! This class is internal to the TSDuck library and cannot be called by applications.
        //! @ingroup libtsduck plugin
        //!
        class JointTermination: public PluginThread
        {
            TS_NOBUILD_NOCOPY(JointTermination);
        public:
            //!
            //! Constructor.
            //! @param [in] options Command line options for tsp.
            //! @param [in] type Plugin type.
            //! @param [in] pl_options Command line options for this plugin.
            //! @param [in] attributes Creation attributes for the thread executing this plugin.
            //! @param [in,out] global_mutex Global mutex to synchronize access to the packet buffer.
            //! @param [in,out] report Where to report logs.
            //!
            JointTermination(const TSProcessorArgs& options,
                             PluginType type,
                             const PluginOptions& pl_options,
                             const ThreadAttributes& attributes,
                             std::recursive_mutex& global_mutex,
                             Report* report);

            // Implementation of "joint termination", inherited from TSP.
            virtual void useJointTermination(bool on) override;
            virtual void jointTerminate() override;
            virtual bool useJointTermination() const override;
            virtual bool thisJointTerminated() const override;

        protected:
            std::recursive_mutex& _global_mutex;
            const TSProcessorArgs& _options;

            //!
            //! Get the packet number after which the "joint termination" must be applied.
            //! @return The packet number after which the "joint termination" must be applied.
            //! If no "joint termination" applies, return the maximum int value.
            //!
            PacketCounter totalPacketsBeforeJointTermination() const;

        private:
            bool _use_jt = false;        // Use "joint termination"
            bool _jt_completed = false;  // Completed, for "joint termination"

            // The following static private data must be accessed exclusively under the protection of the global mutex.
            static int           _jt_users;        // Nb plugins using "joint termination"
            static int           _jt_remaining;    // Nb pluging using jt but not yet completed
            static PacketCounter _jt_highest_pkt;  // Highest pkt# for completed jt plugins
        };
    }
}
