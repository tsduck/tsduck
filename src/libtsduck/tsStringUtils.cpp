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
//  Utility routines for strings
//
//----------------------------------------------------------------------------

#include "tsStringUtils.h"
#include <cctype>
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Trim leading & trailing spaces in a string
//----------------------------------------------------------------------------

std::string& ts::Trim (std::string& str, bool leading, bool trailing)
{
    if (leading) {
        size_t index = 0;
        const size_t len = str.length();
        while (index < len && IsSpace(str[index])) {
            index++;
        }
        str.erase (0, index);
    }
    if (trailing) {
        size_t index = str.length();
        while (index > 0 && IsSpace(str[index-1])) {
            index--;
        }
        str.erase (index);
    }
    return str;
}


//----------------------------------------------------------------------------
// Return a copy of str where leading & trailing spaces are trimmed.
//----------------------------------------------------------------------------

std::string ts::ReturnTrim(const std::string& str, bool leading, bool trailing)
{
    std::string result(str);
    Trim(result, leading, trailing);
    return result;
}


//----------------------------------------------------------------------------
// Return a copy of str where all occurences of substr are removed.
//----------------------------------------------------------------------------

std::string ts::ReturnRemoveSubstring (const std::string& str, const char* substr)
{
    std::string result (str);
    RemoveSubstring (result, substr);
    return result;
}

std::string ts::ReturnRemoveSubstring (const std::string& str, const std::string& substr)
{
    std::string result (str);
    RemoveSubstring (result, substr);
    return result;
}


//----------------------------------------------------------------------------
// Remove all occurences of substr from str.
//----------------------------------------------------------------------------

std::string& ts::RemoveSubstring(std::string& str, const char* substr)
{
    const size_t len = ::strlen(substr);  // Flawfinder: ignore: strlen()
    if (len > 0) {
        size_t index;
        while (!str.empty() && (index = str.find(substr)) != std::string::npos) {
            str.erase(index, len);
        }
    }
    return str;
}


//----------------------------------------------------------------------------
// Substitute all occurences of a string with another one.
//----------------------------------------------------------------------------

std::string& ts::SubstituteAll(std::string& str, const std::string& value, const std::string& replace)
{
    // Filter out degenerated cases.
    if (!str.empty() && !value.empty()) {
        size_t start = 0;
        size_t index;
        while ((index = str.find (value, start)) != std::string::npos) {
            str.replace (index, value.length(), replace);
            start = index + replace.length();
        }
    }
    return str;
}


//----------------------------------------------------------------------------
// Return a copy of a string where all occurences of a string are substituted.
//----------------------------------------------------------------------------

std::string ts::ReturnSubstituteAll(const std::string& str, const std::string& value, const std::string& replace)
{
    std::string result(str);
    SubstituteAll(result, value, replace);
    return result;
}


//----------------------------------------------------------------------------
// Check if a string starts with a specified prefix.
//----------------------------------------------------------------------------

bool ts::StartWith (const std::string& s, const char* prefix)
{
    if (prefix == 0) {
        return false;
    }
    const size_t max = s.length();
    for (size_t i = 0; prefix[i] != '\0'; ++i) {
        if (i >= max || prefix[i] != s.at(i)) {
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Check if a string starts with a specified prefix, case-insensitive.
//----------------------------------------------------------------------------

bool ts::StartWithInsensitive (const std::string& s, const char* prefix)
{
    if (prefix == 0) {
        return false;
    }
    const size_t max = s.length();
    for (size_t i = 0; prefix[i] != '\0'; ++i) {
        if (i >= max || std::tolower(prefix[i]) != std::tolower(s.at(i))) {
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Check if a string ends with a specified suffix.
//----------------------------------------------------------------------------

bool ts::EndWith (const std::string& s, const char* suffix)
{
    if (suffix == 0) {
        return false;
    }
    size_t iString = s.length();
    size_t iSuffix = ::strlen(suffix);  // Flawfinder: ignore: strlen()
    if (iString < iSuffix) {
        return false;
    }
    while (iSuffix > 0) {
        --iSuffix;
        --iString;
        if (suffix[iSuffix] != s.at(iString)) {
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Check if a string ends with a specified suffix, case-insensitive.
//----------------------------------------------------------------------------

bool ts::EndWithInsensitive (const std::string& s, const char* suffix)
{
    if (suffix == 0) {
        return false;
    }
    size_t iString = s.length();
    size_t iSuffix = ::strlen(suffix);  // Flawfinder: ignore: strlen()
    if (iString < iSuffix) {
        return false;
    }
    while (iSuffix > 0) {
        --iSuffix;
        --iString;
        if (std::tolower(suffix[iSuffix]) != std::tolower(s.at(iString))) {
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Return a left-justified (padded and optionally truncated) string.
//----------------------------------------------------------------------------

std::string ts::JustifyLeft (const std::string& s, size_t width, char pad, bool truncate)
{
    size_t len = s.length();

    if (truncate && len > width)
        return s.substr (0, width);
    else if (len >= width)
        return s;
    else
        return s + std::string (width - len, pad);
}


//----------------------------------------------------------------------------
// Return a right-justified (padded and optionally truncated) string.
//----------------------------------------------------------------------------

std::string ts::JustifyRight (const std::string& s, size_t width, char pad, bool truncate)
{
    size_t len = s.length();

    if (truncate && len > width)
        return s.substr (len - width);
    else if (len >= width)
        return s;
    else
        return std::string (width - len, pad) + s;
}


//----------------------------------------------------------------------------
//! Return a centered-justified (padded and optionally truncated) string.
//----------------------------------------------------------------------------

std::string ts::JustifyCentered(const std::string& str, size_t width, char pad, bool truncate)
{
    size_t len = str.length();

    if (truncate && len > width) {
        return str.substr (0, width);
    }
    else if (len >= width) {
        return str;
    }
    else {
        const size_t leftSize = (width - len) / 2;
        const size_t rightSize = width - len - leftSize;
        return std::string (leftSize, pad) + str + std::string (rightSize, pad);
    }
}


//----------------------------------------------------------------------------
// Return a justified string. Pad in the middle.
//----------------------------------------------------------------------------

std::string ts::Justify (const std::string& left, const std::string& right, size_t width, char pad)
{
    size_t len = left.length() + right.length();

    if (len >= width)
        return left + right;
    else
        return left + std::string (width - len, pad) + right;
}


//----------------------------------------------------------------------------
// Check if two strings are identical, case-insensitive and ignoring blanks
//----------------------------------------------------------------------------

bool ts::SimilarStrings (const std::string& a, const std::string& b)
{
    return SimilarStrings (a, b.c_str(), b.length());
}

bool ts::SimilarStrings (const std::string& a, const void* baddr, size_t blen)
{
    assert (baddr != 0);

    const std::locale loc;
    const char* b = reinterpret_cast <const char*> (baddr);
    size_t alen = a.length();
    size_t ai = 0;
    size_t bi = 0;

    for (;;) {
        // Skip spaces
        while (ai < alen && std::isspace (a[ai], loc)) {
            ai++;
        }
        while (bi < blen && std::isspace (b[bi], loc)) {
            bi++;
        }
        if (ai >= alen && bi >= blen) {
            return true;
        }
        if (ai >= alen || bi >= blen || std::tolower (a[ai], loc) != std::tolower (b[bi], loc)) {
            return false;
        }
        ai++;
        bi++;
    }
}


//----------------------------------------------------------------------------
// Interpret a string as a sequence of hexadecimal digits (ignore blanks).
// Place the result into a vector of bytes.
// Return true on success, false on error (invalid hexa format).
//----------------------------------------------------------------------------

bool ts::HexaDecode (std::vector<uint8_t>& result, const char* hexa_string)
{
    result.clear();
    return HexaDecodeAndAppend (result, hexa_string);
}

bool ts::HexaDecode (std::vector<uint8_t>& result, const std::string& hexa_string)
{
    result.clear();
    return HexaDecodeAndAppend (result, hexa_string.c_str());
}

bool ts::HexaDecodeAndAppend (std::vector<uint8_t>& result, const std::string& hexa_string)
{
    return HexaDecodeAndAppend (result, hexa_string.c_str());
}

bool ts::HexaDecodeAndAppend (std::vector<uint8_t>& result, const char* hexa_string)
{
    result.reserve(result.size() + ::strlen(hexa_string) / 2);  // Flawfinder: ignore: strlen()
    const std::locale loc;
    bool got_first_nibble = false;
    uint8_t byte = 0;

    for (const char* p = hexa_string; *p != '\0'; ++p) {
        uint8_t nibble;
        if (std::isspace (*p, loc)) {
            continue;
        }
        else if (*p >= '0' && *p <= '9') {
            nibble = *p - '0';
        }
        else if (*p >= 'A' && *p <= 'F') {
            nibble = *p - 'A' + 10;
        }
        else if (*p >= 'a' && *p <= 'f') {
            nibble = *p - 'a' + 10;
        }
        else {
            return false;
        }
        if (got_first_nibble) {
            result.push_back (byte | nibble);
        }
        else {
            byte = nibble << 4;
        }
        got_first_nibble = !got_first_nibble;
    }

    return !got_first_nibble;
}


//----------------------------------------------------------------------------
// Check if a character is a space.
//----------------------------------------------------------------------------

bool ts::IsSpace(int c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}


//----------------------------------------------------------------------------
// Update a string to lower/upper case. Return a reference to string parameter.
//----------------------------------------------------------------------------

std::string& ts::ToLowerCase(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), ToLower);
    return s;
}

std::string& ts::ToUpperCase(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), ToUpper);
    return s;
}


//----------------------------------------------------------------------------
// Return a lower/upper case copy of a string
//----------------------------------------------------------------------------

std::string ts::LowerCaseValue(const std::string& s)
{
    std::string res(s.length(), ' ');
    std::transform(s.begin(), s.end(), res.begin(), ToLower);
    return res;
}

std::string ts::UpperCaseValue(const std::string& s)
{
    std::string res(s.length(), ' ');
    std::transform(s.begin(), s.end(), res.begin(), ToUpper);
    return res;
}


//----------------------------------------------------------------------------
// Remove all occurences of character c in string s. Return a reference to string parameter.
//----------------------------------------------------------------------------

std::string& ts::RemoveCharacter(std::string& s, char c)
{
    s.erase(std::remove(s.begin(), s.end(), c), s.end());
    return s;
}


//----------------------------------------------------------------------------
// Remove prefix/suffix in string.
//----------------------------------------------------------------------------

std::string& ts::RemovePrefix(std::string& s, const std::string& prefix)
{
    const size_t pl = prefix.length();
    if (s.length() >= pl && s.substr (0, pl) == prefix) {
        s.erase (0, pl);
    }
    return s;
}

std::string& ts::RemoveSuffix(std::string& s, const std::string& suffix)
{
    const size_t l = s.length();
    const size_t sl = suffix.length();
    if (l >= sl && s.substr (l - sl, sl) == suffix) {
        s.erase (l - sl, sl);
    }
    return s;
}

std::string ts::ReturnRemovePrefix(const std::string& s, const std::string& prefix)
{
    std::string res(s);
    RemovePrefix(res, prefix);
    return res;
}

std::string ts::ReturnRemoveSuffix(const std::string& s, const std::string& suffix)
{
    std::string res(s);
    RemoveSuffix(res, suffix);
    return res;
}
