//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//!  Input switch (tsswitch) output plugin executor thread.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPluginThread.h"

namespace ts {
    //!
    //! Input switch (tsswitch) namespace.
    //!
    namespace tsswitch {

        class Core;
        class Options;

        //!
        //! Execution context of a tsswitch output plugin.
        //! @ingroup plugin
        //!
        class OutputExecutor : public PluginThread
        {
            TS_NOBUILD_NOCOPY(OutputExecutor);
        public:
            //!
            //! Constructor.
            //! @param [in,out] core Command core instance.
            //! @param [in,out] opt Command line options.
            //! @param [in,out] log Log report.
            //!
            OutputExecutor(Core& core, Options& opt, Report& log);

            //!
            //! Destructor.
            //!
            virtual ~OutputExecutor();

            //!
            //! Request the termination of the thread.
            //! Actual termination will occur after completion of the current output operation.
            //!
            void terminateOutput() { _terminate = true; }

            // Implementation of TSP. We do not use "joint termination" in tsswitch.
            virtual void useJointTermination(bool) override;
            virtual void jointTerminate() override;
            virtual bool useJointTermination() const override;
            virtual bool thisJointTerminated() const override;

        private:
            Core&         _core;       // Application core.
            OutputPlugin* _output;     // Plugin API.
            volatile bool _terminate;  // Termination request.

            // Implementation of Thread.
            virtual void main() override;
        };
    }
}
