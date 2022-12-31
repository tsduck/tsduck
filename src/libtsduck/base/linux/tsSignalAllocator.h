//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
//!
//!  @file
//!  Allocate POSIX real-time signal numbers (Linux-specific).
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsSingletonManager.h"
#include "tsMutex.h"

namespace ts {
    //!
    //! Allocate POSIX real-time signal numbers (Linux-specific).
    //! @ingroup unix
    //!
    class TSDUCKDLL SignalAllocator
    {
        // This class is a singleton. Use static Instance() method.
        TS_DECLARE_SINGLETON(SignalAllocator);

    public:
        //!
        //! Allocate a new signal number.
        //! @return A POSIX real-time signal number or -1 if none available.
        //!
        int allocate();

        //!
        //! Release a signal number.
        //! @param [in] sig A POSIX real-time signal number.
        //!
        void release(int sig);

    private:
        // Private members:
        const int         _signal_min;
        const int         _signal_max;
        const size_t      _signal_count;
        Mutex             _mutex;
        std::vector<bool> _signals;
    };
}
