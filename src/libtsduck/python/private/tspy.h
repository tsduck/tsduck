//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup python
//!  Base definitions for the TSDuck Python bindings (C++ implementation).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

//!
//! @hideinitializer
//! Attribute to export a function to Python.
//!
#define TSDUCKPY \
    TS_GCC_NOWARNING(missing-prototypes) \
    TS_LLVM_NOWARNING(missing-prototypes) \
    extern "C" TSDUCKDLL

namespace ts {
    //!
    //! Namespace for internal utilities to support Python bindings.
    //!
    namespace py {
        //!
        //! Convert a UTF-16 buffer in a UString.
        //! @param [in] buffer Address of a buffer with UTF-16 content.
        //! @param [in] size Size in bytes of the buffer.
        //! @return The converted string.
        //!
        UString ToString(const uint8_t* buffer, size_t size);

        //!
        //! Convert a UTF-16 buffer in a list of UString.
        //! The various strings in the buffer are separated with 0xFFFF code points (invalid UTF-16 value)
        //! @param [in] buffer Address of a buffer with UTF-16 content.
        //! @param [in] size Size in bytes of the buffer.
        //! @return The converted strings.
        //!
        UStringList ToStringList(const uint8_t* buffer, size_t size);

        //!
        //! Convert a string into a UTF-16 buffer.
        //! @param [in] str The initial string.
        //! @param [out] buffer Address of a buffer where the string is returned in UTF-16 format.
        //! @param [in,out] size Initial/maximum size in bytes of the buffer. Upon return, contains the written size in bytes.
        //!
        void FromString(const UString& str, uint8_t* buffer, size_t* size);
    }
}
