//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
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
#include "tsMutex.h"

namespace ts {
    namespace tsp {
        //!
        //! Implementation of "Joint Termination" in the Transport stream processor.
        //! This is a subclass of ts::TSP and a superclass of all plugin executors.
        //! This class is internal to the TSDuck library and cannot be called by applications.
        //! @ingroup plugin
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
                             Mutex& global_mutex,
                             Report* report);

            // Implementation of "joint termination", inherited from TSP.
            virtual void useJointTermination(bool on) override;
            virtual void jointTerminate() override;
            virtual bool useJointTermination() const override;
            virtual bool thisJointTerminated() const override;

        protected:
            Mutex& _global_mutex;
            const TSProcessorArgs& _options;

            //!
            //! Get the packet number after which the "joint termination" must be applied.
            //! @return The packet number after which the "joint termination" must be applied.
            //! If no "joint termination" applies, return the maximum int value.
            //!
            PacketCounter totalPacketsBeforeJointTermination() const;

        private:
            bool _use_jt;        // Use "joint termination"
            bool _jt_completed;  // Completed, for "joint termination"

            // The following static private data must be accessed exclusively under the protection of the global mutex.
            static int           _jt_users;         // Nb plugins using "joint termination"
            static int           _jt_remaining;     // Nb pluging using jt but not yet completed
            static PacketCounter _jt_hightest_pkt;  // Highest pkt# for completed jt plugins
        };
    }
}
