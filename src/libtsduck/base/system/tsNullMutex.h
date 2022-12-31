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
//!  Empty mutex implementation.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMutexInterface.h"

namespace ts {
    //!
    //! Empty mutex implementation.
    //! @ingroup thread
    //!
    //! The concrete class ts::NullMutex is an empty mutex implementation
    //! which does nothing and can be used wherever a ts::MutexInterface is
    //! required but no actual synchronization is necessary (non-threaded
    //! applications for instances).
    //!
    class TSDUCKDLL NullMutex: public MutexInterface
    {
    public:
        //!
        //! Acquire the mutex, does nothing but successfully!
        //! @param [in] timeout Maximum number of milliseconds to wait for the mutex. Ignored.
        //! @return Always true.
        //!
        virtual bool acquire(MilliSecond timeout = Infinite) override;

        //!
        //! Release the mutex, does nothing but successfully!
        //! @return Always true.
        //!
        virtual bool release() override;
    };
}
