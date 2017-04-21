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
#include "tsPlatform.h"

namespace ts {

    //!
    //! Base class for all exceptions in TSDuck.
    //!
    class TSDUCKDLL Exception : public std::exception
    {
    private:
        std::string _what;
    public:
        explicit Exception(const std::string&);
        explicit Exception(const std::string&, ErrorCode error);
        virtual ~Exception() throw();
        virtual const char* what() const throw();
    };
}

//!
//! @hideinitializer
//! This macro declares an exception as a subclass of ts::Exception.
//! @param [in] name Name of the exception class.
//!
#define tsDeclareException(name)                                  \
    class name: public ts::Exception                              \
    {                                                             \
    public:                                                       \
        explicit name(const std::string& w) :                     \
            ts::Exception(#name ": " + w)                         \
        {                                                         \
        }                                                         \
        explicit name(const std::string& w, ts::ErrorCode code) : \
            ts::Exception(#name ": " + w, code)                   \
        {                                                         \
        }                                                         \
        explicit name(ts::ErrorCode code) :                       \
            ts::Exception(#name, code)                            \
        {                                                         \
        }                                                         \
    }

// This macro can be used to locate the source of the exception
// in the Exception constructor message string.

#define TS_SRCLOC __FILE__ ":" TS_SLINE ": "

// Some "standard" exceptions

namespace ts {
    tsDeclareException(InvalidValue);
    tsDeclareException(UninitializedVariable);
    tsDeclareException(UnimplementedMethod);
}
