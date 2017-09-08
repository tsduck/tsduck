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
//!  Declare some utilities on Unicode strings.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include <locale>

namespace ts {

    //!
    //! Convert an UTF-8 string into UTF-16.
    //! @param [in] utf8 A string in UTF-8 representation.
    //! @return The equivalent UTF-16 string.
    //!
    TSDUCKDLL std::u16string ToUTF16(const std::string& utf8);

    //!
    //! Convert an UTF-16 string into UTF-8.
    //! @param [in] utf16 A string in UTF-16 representation.
    //! @return The equivalent UTF-8 string.
    //!
    TSDUCKDLL std::string ToUTF8(const std::u16string& utf16);

    //!
    //! Check if two UTF-8 strings are identical.
    //! @param [in] s1 First string to compare.
    //! @param [in] s2 Second string to compare.
    //! @param [in] caseSensitive If true (the default), the comparison is
    //! case-sensitive. When false, the comparison is not case-sensitive.
    //! @param [in] loc The locale into which the operation is performed.
    //! Useful only when @a caseSensitive is false.
    //! The default value is the classic ANSI-C locale.
    //! @return True is @a s1 and @a s2 are identical.
    //!
    TSDUCKDLL bool UTF8Equal(const std::string& s1, const std::string& s2, bool caseSensitive = true, const std::locale& loc = std::locale::classic());

    //!
    //! Check if two UTF-8 strings are identical.
    //! @param [in] s1 First string to compare.
    //! @param [in] s2 Second string to compare.
    //! @param [in] caseSensitive If true (the default), the comparison is
    //! case-sensitive. When false, the comparison is not case-sensitive.
    //! @param [in] loc The locale into which the operation is performed.
    //! Useful only when @a caseSensitive is false.
    //! The default value is the classic ANSI-C locale.
    //! @return True is @a s1 and @a s2 are identical.
    //!
    TSDUCKDLL inline bool UTF8Equal(const char* s1, const char* s2, bool caseSensitive = true, const std::locale& loc = std::locale::classic())
    {
        return UTF8Equal(std::string(s1 == 0 ? "" : s1), std::string(s2 == 0 ? "" : s2), caseSensitive, loc);
    }
}
