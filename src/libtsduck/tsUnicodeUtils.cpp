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
//
//  Utility routines for Unicode strings
//
//----------------------------------------------------------------------------

#include "tsUnicodeUtils.h"
#include <codecvt>
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Convert between UTF-and UTF-16.
//----------------------------------------------------------------------------

// Warning: There is a bug in MSVC 2015 and 2017 (don't know about future releases).
// Need an ugly workaround.

#if defined(_MSC_VER) && _MSC_VER >= 1900

std::u16string ts::ToUTF16(const std::string& utf8)
{
    std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> convert;
    const std::basic_string<int16_t> result(convert.from_bytes(utf8));
    return std::u16string(reinterpret_cast<const char16_t*>(result.data()), result.size());
}

std::string ts::ToUTF8(const std::u16string& utf16)
{
    std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> convert;
    const int16_t* p = reinterpret_cast<const int16_t*>(utf16.data());
    return convert.to_bytes(p, p + utf16.size());
}

#else

std::u16string ts::ToUTF16(const std::string& utf8)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    return convert.from_bytes(utf8);
}

std::string ts::ToUTF8(const std::u16string& utf16)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    return convert.to_bytes(utf16);
}

#endif


//----------------------------------------------------------------------------
// Check if two UTF-8 strings are identical.
//----------------------------------------------------------------------------

bool ts::UTF8Equal(const char* s1, const char* s2, bool caseSensitive, const std::locale& loc)
{
    if (s1 == 0 || s2 == 0) {
        return false;
    }
    else if (s1 == s2) {
        return true;
    }
    else if (caseSensitive) {
        // Not case sensitive, this is a basic string memory comparison.
        return ::strcmp(s1, s2);
    }
    else {
        // Case sensitive comparison. First convert to UTF-16 and compare 16-bit codes.
        // Yes, I know, UTF-16 is not UTF-32...
        const std::u16string u1(ToUTF16(std::string(s1)));
        const std::u16string u2(ToUTF16(std::string(s2)));
        if (u1.length() != u2.length()) {
            return false;
        }
        for (std::u16string::size_type i = 0; i < u1.length(); ++i) {
            if (std::toupper(s1[i], loc) != std::toupper(s2[i], loc)) {
                return false;
            }
        }
        return true;
    }
}
