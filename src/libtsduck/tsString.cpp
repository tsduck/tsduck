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
//  Unicode string.
//
//----------------------------------------------------------------------------

#include "tsString.h"
#include <codecvt>
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Convert between UTF-and UTF-16.
//----------------------------------------------------------------------------

// Warning: There is a bug in MSVC 2015 and 2017 (don't know about future releases).
// Need an ugly workaround.

#if defined(_MSC_VER) && _MSC_VER >= 1900

ts::String ts::String::FromUTF8(const std::string& utf8)
{
    std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> convert;
    const std::basic_string<int16_t> result(convert.from_bytes(utf8));
    return String(reinterpret_cast<const char16_t*>(result.data()), result.size());
}

ts::String ts::String::FromUTF8(const char* utf8)
{
    if (utf8 == 0) {
        return String();
    }
    else {
        std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> convert;
        const std::basic_string<int16_t> result(convert.from_bytes(utf8));
        return String(reinterpret_cast<const char16_t*>(result.data()), result.size());
    }
}

ts::String ts::String::FromUTF8(const char* utf8, size_type count)
{
    if (utf8 == 0) {
        return String();
    }
    else {
        std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> convert;
        const std::basic_string<int16_t> result(convert.from_bytes(utf8, utf8 + count));
        return String(reinterpret_cast<const char16_t*>(result.data()), result.size());
    }
}

std::string ts::String::toUTF8() const
{
    std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> convert;
    const int16_t* p = reinterpret_cast<const int16_t*>(data());
    return convert.to_bytes(p, p + size());
}

#else

ts::String ts::String::FromUTF8(const std::string& utf8)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    return convert.from_bytes(utf8);
}

ts::String ts::String::FromUTF8(const char* utf8)
{
    if (utf8 == 0) {
        return String();
    }
    else {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
        return convert.from_bytes(utf8);
    }
}

ts::String ts::String::FromUTF8(const char* utf8, size_type count)
{
    if (utf8 == 0) {
        return String();
    }
    else {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
        return convert.from_bytes(utf8, utf8 + count);
    }
}

std::string ts::String::toUTF8() const
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    return convert.to_bytes(*this);
}

#endif


//----------------------------------------------------------------------------
// Comparison operator with UTF-8 strings.
//----------------------------------------------------------------------------

bool ts::String::operator==(const std::string& other) const
{
    return operator==(String(other));
}

bool ts::String::operator==(const char* other) const
{
    return other != 0 && operator==(String(other));
}


//----------------------------------------------------------------------------
// Trim leading & trailing spaces in the string
//----------------------------------------------------------------------------

void ts::String::trim(bool leading, bool trailing)
{
    if (trailing) {
        size_t index = length();
        while (index > 0 && IsSpace((*this)[index-1])) {
            index--;
        }
        erase(index);
    }
    if (leading) {
        size_t index = 0;
        const size_t len = length();
        while (index < len && IsSpace((*this)[index])) {
            index++;
        }
        erase(0, index);
    }
}

ts::String ts::String::toTrimmed(bool leading, bool trailing) const
{
    String result(*this);
    result.trim(leading, trailing);
    return result;
}


//----------------------------------------------------------------------------
// Return a lower/upper-case version of the string.
//----------------------------------------------------------------------------

void ts::String::convertToLower()
{
    const size_t len = size();
    for (size_t i = 0; i < len; ++i) {
        (*this)[i] = ToLower((*this)[i]);
    }
}

void ts::String::convertToUpper()
{
    const size_t len = size();
    for (size_t i = 0; i < len; ++i) {
        (*this)[i] = ToUpper((*this)[i]);
    }
}

ts::String ts::String::toLower() const
{
    String result(*this);
    result.convertToLower();
    return result;
}

ts::String ts::String::toUpper() const
{
    String result(*this);
    result.convertToUpper();
    return result;
}


//----------------------------------------------------------------------------
// Remove all occurences of a substring.
//----------------------------------------------------------------------------

void ts::String::remove(const String& substr)
{
    const size_t len = substr.size();
    if (len > 0) {
        size_t index;
        while (!empty() && (index = find(substr)) != npos) {
            erase(index, len);
        }
    }
}

ts::String ts::String::toRemoved(const String& substr) const
{
    String result(*this);
    result.remove(substr);
    return result;
}


//----------------------------------------------------------------------------
// Substitute all occurences of a string with another one.
//----------------------------------------------------------------------------

void ts::String::substitute(const String& value, const String& replacement)
{
    // Filter out degenerated cases.
    if (!empty() && !value.empty()) {
        size_t start = 0;
        size_t index;
        while ((index = find(value, start)) != npos) {
            replace(index, value.length(), replacement);
            start = index + replacement.length();
        }
    }
}

ts::String ts::String::toSubstituted(const String& value, const String& replacement) const
{
    String result(*this);
    result.substitute(value, replacement);
    return result;
}
