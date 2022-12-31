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
//!  @ingroup cpp
//!  Handle some fatal situations.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {
    //!
    //! Handle a fatal error.
    //! An emergency message is output and the application is terminated.
    //! @param [in] message Address of an emergency error message to output.
    //! @param [in] length Length of @a message. The caller must specify @a length
    //! in a static way. In that kind of fatal error, we can't even dare to call strlen().
    //!
    [[noreturn]] TSDUCKDLL void FatalError(const char* message, size_t length);

    //!
    //! Handle fatal memory allocation failure.
    //! Out of virtual memory, very dangerous situation, really can't
    //! recover from that, need to abort immediately. An emergency error
    //! message is output and the application is terminated.
    //!
    [[noreturn]] TSDUCKDLL void FatalMemoryAllocation();

    //!
    //! Check the value of a pointer and abort the application when zero.
    //! This function is typically after a new.
    //! @param [in] ptr The pointer to check.
    //! @see FatalMemoryAllocation()
    //!
    TSDUCKDLL inline void CheckNonNull(const void* ptr)
    {
        if (ptr == nullptr) {
            FatalMemoryAllocation();
        }
    }
}
