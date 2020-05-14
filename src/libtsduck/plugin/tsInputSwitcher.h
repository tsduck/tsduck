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
//!  Implementation of the input plugin switcher (command tsswitch).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPluginEventHandlerRegistry.h"
#include "tsInputSwitcherArgs.h"

namespace ts {
    //!
    //! Implementation of the input plugin switcher.
    //! This class is used by the @a tsswitch utility.
    //! It can also be used in other applications to switch between input plugins.
    //! @ingroup plugin
    //!
    class TSDUCKDLL InputSwitcher: public PluginEventHandlerRegistry
    {
        TS_NOBUILD_NOCOPY(InputSwitcher);
    public:
        //!
        //! Constructor.
        //! The complete input switching session is performed in the constructor.
        //! @param [in] args Arguments and options.
        //! @param [in,out] report Where to report errors, logs, etc.
        //! This object will be used concurrently by all plugin execution threads.
        //! Consequently, it must be thread-safe. For performance reasons, it should
        //! be asynchronous (see for instance class AsyncReport).
        //!
        InputSwitcher(const InputSwitcherArgs& args, Report& report);

        //!
        //! Check if the session (completely run in the constructor) was successful.
        //! @return True on success, false on failure to start.
        //!
        bool success() const { return _success; }

    private:
        bool _success;
    };
}
