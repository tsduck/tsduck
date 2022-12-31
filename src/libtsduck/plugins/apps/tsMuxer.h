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
//!  Implementation of the TS multiplexer (command tsmux).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPluginEventHandlerRegistry.h"
#include "tsMuxerArgs.h"

namespace ts {

    // Used in private part.
    namespace tsmux {
        class Core;
    }

    //!
    //! Implementation of the TS multiplexer.
    //! This class is used by the @a tsmux utility.
    //! It can also be used in other applications to multiplex input streams.
    //! @ingroup plugin
    //!
    class TSDUCKDLL Muxer: public PluginEventHandlerRegistry
    {
        TS_NOBUILD_NOCOPY(Muxer);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors, logs, etc.
        //! This object will be used concurrently by all plugin execution threads.
        //! Consequently, it must be thread-safe. For performance reasons, it should
        //! be asynchronous (see for instance class AsyncReport).
        //!
        Muxer(Report& report);

        //!
        //! Destructor.
        //! It waits for termination of the session if it is running.
        //!
        ~Muxer();

        //!
        //! Get a reference to the report object for the multiplexer.
        //! @return A reference to the report object.
        //!
        Report& report() const { return _report; }

        //!
        //! Start the multiplexer session.
        //! @param [in] args Arguments and options.
        //! @return True on success, false on failure to start.
        //!
        bool start(const MuxerArgs& args);

        //!
        //! Stop the multiplexer.
        //!
        void stop();

        //!
        //! Suspend the calling thread until the multiplexer is completed.
        //!
        void waitForTermination();

    private:
        Report&      _report;
        MuxerArgs    _args;
        tsmux::Core* _core;
    };
}
