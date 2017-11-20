//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!  Base class for all exceptions in TSDuck.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! Base class for all exceptions in TSDuck.
    //!
    class TSDUCKDLL Exception : public std::exception
    {
    private:
        UString _what;
        mutable std::string _utf8;
    public:
        //!
        //! Constructor.
        //! @param [in] message Error message for the exception.
        //!
        explicit Exception(const UString& message);

        //!
        //! Constructor.
        //! @param [in] message Error message for the exception.
        //! @param [in] error System error code causing the exception.
        //!
        Exception(const UString& message, ErrorCode error);

        //!
        //! Destructor.
        //!
        virtual ~Exception() throw();

        //!
        //! Get the error message as a C-string.
        //! @return The error message as a C-string (valid as long as this instance exists).
        //!
        virtual const char* what() const throw() override;
    };
}

//!
//! @hideinitializer
//! This macro declares an exception as a subclass of ts::Exception.
//! @param [in] name Name of the exception class.
//!
#define TS_DECLARE_EXCEPTION(name)                                \
    class name: public ts::Exception                              \
    {                                                             \
    public:                                                       \
        /** Constructor.                                       */ \
        /** @param [in] w Error message for the exception.     */ \
        explicit name(const ts::UString& w) :                     \
            ts::Exception(#name ": " + w)                         \
        {                                                         \
        }                                                         \
        /** Constructor.                                       */ \
        /** @param [in] w Error message for the exception.     */ \
        /** @param [in] code System error code.                */ \
        explicit name(const ts::UString& w, ts::ErrorCode code) : \
            ts::Exception(#name ": " + w, code)                   \
        {                                                         \
        }                                                         \
        /** Constructor.                                       */ \
        /** @param [in] code System error code.                */ \
        explicit name(ts::ErrorCode code) :                       \
            ts::Exception(#name, code)                            \
        {                                                         \
        }                                                         \
    }

//!
//! Locate the source of the exception in the Exception constructor message string.
//!
#define TS_SRCLOC __FILE__ ":" TS_SLINE ": "

//
// Some "standard" exceptions
//
namespace ts {
    //!
    //! Exception for generic invalid value error.
    //!
    TS_DECLARE_EXCEPTION(InvalidValue);
    //!
    //! Uninitialized variable error.
    //!
    TS_DECLARE_EXCEPTION(UninitializedVariable);
    //!
    //! Unimplemented method error.
    //!
    TS_DECLARE_EXCEPTION(UnimplementedMethod);
    //!
    //! Implementation error.
    //!
    TS_DECLARE_EXCEPTION(ImplementationError);
}
