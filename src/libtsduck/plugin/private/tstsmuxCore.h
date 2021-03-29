//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//!  Multiplexer (tsmux) core engine.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsThread.h"
#include "tsMuxerArgs.h"
#include "tstsmuxInputExecutor.h"
#include "tstsmuxOutputExecutor.h"

namespace ts {
    namespace tsmux {
        //!
        //! Multiplexer (tsmux) core engine.
        //! @ingroup plugin
        //!
        class Core: private Thread
        {
            TS_NOBUILD_NOCOPY(Core);
        public:
            //!
            //! Constructor.
            //! @param [in] opt Command line options.
            //! @param [in] handlers Registry of plugin event handlers.
            //! @param [in,out] log Log report.
            //!
            Core(const MuxerArgs& opt, const PluginEventHandlerRegistry& handlers, Report& log);

            //!
            //! Destructor.
            //!
            virtual ~Core() override;

            //!
            //! Start the @c tsmux processing.
            //! @return True on success, false on error.
            //!
            bool start();

            //!
            //! Stop the @c tsmux processing.
            //!
            void stop();

            //!
            //! Wait for completion of all plugin threads.
            //!
            void waitForTermination();

        private:
            Report&             _log;        // Asynchronous log report.
            const MuxerArgs&    _opt;        // Command line options.
            volatile bool       _terminate;  // Termination request.
            BitRate             _bitrate;    // Constant output bitrate.
            InputExecutorVector _inputs;     // Input plugins threads.
            OutputExecutor      _output;     // Output plugin thread.

            // Implementation of Thread.
            virtual void main() override;
        };
    }
}
