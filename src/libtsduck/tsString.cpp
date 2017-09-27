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
        size_type index = length();
        while (index > 0 && IsSpace((*this)[index-1])) {
            index--;
        }
        erase(index);
    }
    if (leading) {
        size_type index = 0;
        const size_type len = length();
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
    const size_type len = size();
    for (size_type i = 0; i < len; ++i) {
        (*this)[i] = ToLower((*this)[i]);
    }
}

void ts::String::convertToUpper()
{
    const size_type len = size();
    for (size_type i = 0; i < len; ++i) {
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
    const size_type len = substr.size();
    if (len > 0) {
        size_type index;
        while (!empty() && (index = find(substr)) != npos) {
            erase(index, len);
        }
    }
}

void ts::String::remove(Char c)
{
    erase(std::remove(begin(), end(), c), end());
}

ts::String ts::String::toRemoved(const String& substr) const
{
    String result(*this);
    result.remove(substr);
    return result;
}

ts::String ts::String::toRemoved(Char c) const
{
    String result(*this);
    result.remove(c);
    return result;
}


//----------------------------------------------------------------------------
// Substitute all occurences of a string with another one.
//----------------------------------------------------------------------------

void ts::String::substitute(const String& value, const String& replacement)
{
    // Filter out degenerated cases.
    if (!empty() && !value.empty()) {
        size_type start = 0;
        size_type index;
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


//----------------------------------------------------------------------------
// Prefix / suffix checking.
//----------------------------------------------------------------------------

void ts::String::removePrefix(const String& prefix, CaseSensitivity cs)
{
    if (startWith(prefix, cs)) {
        erase(0, prefix.length());
    }
}

void ts::String::removeSuffix(const String& suffix, CaseSensitivity cs)
{
    if (endWith(suffix, cs)) {
        assert(length() >= suffix.length());
        erase(length() - suffix.length());
    }
}

ts::String ts::String::toRemovedPrefix(const String& prefix, CaseSensitivity cs) const
{
    String result(*this);
    result.removePrefix(prefix, cs);
    return result;
}

ts::String ts::String::toRemovedSuffix(const String& suffix, CaseSensitivity cs) const
{
    String result(*this);
    result.removeSuffix(suffix, cs);
    return result;
}

bool ts::String::startWith(const String& prefix, CaseSensitivity cs) const
{
    const size_type len = length();
    const size_type sublen = prefix.length();

    if (len < sublen) {
        return false;
    }

    switch (cs) {
        case CASE_SENSITIVE: {
            return compare(0, sublen, prefix) == 0;
        }
        case CASE_INSENSITIVE: {
            for (size_type i = 0; i < sublen; ++i) {
                if (ToLower(at(i)) != ToLower(prefix.at(i))) {
                    return false;
                }
            }
            return true;
        }
        default: {
            assert(false);
            return false;
        }
    }
}

bool ts::String::endWith(const String& suffix, CaseSensitivity cs) const
{
    size_type iString = length();
    size_type iSuffix = suffix.length();

    if (iString < iSuffix) {
        return false;
    }

    switch (cs) {
        case CASE_SENSITIVE: {
            return compare(iString - iSuffix, iSuffix, suffix) == 0;
        }
        case CASE_INSENSITIVE: {
            while (iSuffix > 0) {
                --iSuffix;
                --iString;
                if (ToLower(at(iString)) != ToLower(suffix.at(iSuffix))) {
                    return false;
                }
            }
            return true;
        }
        default: {
            assert(false);
            return false;
        }
    }
}


//----------------------------------------------------------------------------
// Left-justify (pad and optionally truncate) string.
//----------------------------------------------------------------------------

void ts::String::justifyLeft(size_type width, Char pad, bool truncate)
{
    const size_type len = length();
    if (truncate && len > width) {
        erase(width);
    }
    else if (len < width) {
        append(width - len, pad);
    }
}

ts::String ts::String::toJustifiedLeft(size_type width, Char pad, bool truncate) const
{
    String result(*this);
    result.justifyLeft(width, pad, truncate);
    return result;
}


//----------------------------------------------------------------------------
// Right-justified (pad and optionally truncate) string.
//----------------------------------------------------------------------------

void ts::String::justifyRight(size_type width, Char pad, bool truncate)
{
    const size_type len = length();
    if (truncate && len > width) {
        erase(0, len - width);
    }
    else if (len < width) {
        insert(0, width - len, pad);
    }
}

ts::String ts::String::toJustifiedRight(size_type width, Char pad, bool truncate) const
{
    String result(*this);
    result.justifyRight(width, pad, truncate);
    return result;
}


//----------------------------------------------------------------------------
// Centered-justified (pad and optionally truncate) string.
//----------------------------------------------------------------------------

void ts::String::justifyCentered(size_type width, Char pad, bool truncate)
{
    const size_type len = length();
    if (truncate && len > width) {
        erase(width);
    }
    else if (len < width) {
        const size_type leftSize = (width - len) / 2;
        const size_type rightSize = width - len - leftSize;
        insert(0, leftSize, pad);
        append(rightSize, pad);
    }
}

ts::String ts::String::toJustifiedCentered(size_type width, Char pad, bool truncate) const
{
    String result(*this);
    result.justifyCentered(width, pad, truncate);
    return result;
}


//----------------------------------------------------------------------------
// Justify string, pad in the middle.
//----------------------------------------------------------------------------

void ts::String::justify(const String& right, size_type width, Char pad)
{
    const size_type len = length() + right.length();
    if (len < width) {
        append(width - len, pad);
    }
    append(right);
}

ts::String ts::String::toJustified(const String& right, size_type width, Char pad) const
{
    String result(*this);
    result.justify(right, width, pad);
    return result;
}


//----------------------------------------------------------------------------
// Format a boolean value in various standard representation.
//----------------------------------------------------------------------------

ts::String ts::String::YesNo(bool b)
{
    return FromUTF8(b ? "yes" : "no");
}

ts::String ts::String::TrueFalse(bool b)
{
    return FromUTF8(b ? "true" : "false");
}

ts::String ts::String::OnOff(bool b)
{
    return FromUTF8(b ? "on" : "off");
}


//----------------------------------------------------------------------------
// Check if two strings are identical, case-insensitive and ignoring blanks
//----------------------------------------------------------------------------

bool ts::String::similar(const String& other) const
{
    const size_type alen = length();
    const size_type blen = other.length();
    size_type ai = 0;
    size_type bi = 0;

    for (;;) {
        // Skip spaces
        while (ai < alen && IsSpace(at(ai))) {
            ai++;
        }
        while (bi < blen && IsSpace(other.at(bi))) {
            bi++;
        }
        if (ai >= alen && bi >= blen) {
            return true;
        }
        if (ai >= alen || bi >= blen || ToLower(at(ai)) != ToLower(other.at(bi))) {
            return false;
        }
        ai++;
        bi++;
    }
}

bool ts::String::similar(const void* addr, size_type size) const
{
    return addr != 0 && similar(FromUTF8(reinterpret_cast<const char*>(addr), size));
}
