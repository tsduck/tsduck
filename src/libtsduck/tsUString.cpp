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

#include "tsUString.h"
#include "tsByteBlock.h"
#include "tsDVBCharsetSingleByte.h"
#include "tsDVBCharsetUTF8.h"
#include <codecvt>
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Convert between UTF-and UTF-16.
//----------------------------------------------------------------------------

// Warning: There is a bug in MSVC 2015 and 2017 (don't know about future releases).
// Need an ugly workaround.

#if defined(_MSC_VER) && _MSC_VER >= 1900

ts::UString ts::UString::FromUTF8(const std::string& utf8)
{
    std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> convert;
    const std::basic_string<int16_t> result(convert.from_bytes(utf8));
    return UString(reinterpret_cast<const char16_t*>(result.data()), result.size());
}

ts::UString ts::UString::FromUTF8(const char* utf8)
{
    if (utf8 == 0) {
        return UString();
    }
    else {
        std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> convert;
        const std::basic_string<int16_t> result(convert.from_bytes(utf8));
        return UString(reinterpret_cast<const char16_t*>(result.data()), result.size());
    }
}

ts::UString ts::UString::FromUTF8(const char* utf8, size_type count)
{
    if (utf8 == 0) {
        return UString();
    }
    else {
        std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> convert;
        const std::basic_string<int16_t> result(convert.from_bytes(utf8, utf8 + count));
        return UString(reinterpret_cast<const char16_t*>(result.data()), result.size());
    }
}

std::string ts::UString::toUTF8() const
{
    std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> convert;
    const int16_t* p = reinterpret_cast<const int16_t*>(data());
    return convert.to_bytes(p, p + size());
}

#else

ts::UString ts::UString::FromUTF8(const std::string& utf8)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    return convert.from_bytes(utf8);
}

ts::UString ts::UString::FromUTF8(const char* utf8)
{
    if (utf8 == 0) {
        return UString();
    }
    else {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
        return convert.from_bytes(utf8);
    }
}

ts::UString ts::UString::FromUTF8(const char* utf8, size_type count)
{
    if (utf8 == 0) {
        return UString();
    }
    else {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
        return convert.from_bytes(utf8, utf8 + count);
    }
}

std::string ts::UString::toUTF8() const
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
    return convert.to_bytes(*this);
}

#endif


//----------------------------------------------------------------------------
// Comparison operator with UTF-8 strings.
//----------------------------------------------------------------------------

bool ts::UString::operator==(const std::string& other) const
{
    return operator==(UString(other));
}

bool ts::UString::operator==(const char* other) const
{
    return other != 0 && operator==(UString(other));
}


//----------------------------------------------------------------------------
// Trim leading & trailing spaces in the string
//----------------------------------------------------------------------------

void ts::UString::trim(bool leading, bool trailing)
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

ts::UString ts::UString::toTrimmed(bool leading, bool trailing) const
{
    UString result(*this);
    result.trim(leading, trailing);
    return result;
}


//----------------------------------------------------------------------------
// Return a lower/upper-case version of the string.
//----------------------------------------------------------------------------

void ts::UString::convertToLower()
{
    const size_type len = size();
    for (size_type i = 0; i < len; ++i) {
        (*this)[i] = ToLower((*this)[i]);
    }
}

void ts::UString::convertToUpper()
{
    const size_type len = size();
    for (size_type i = 0; i < len; ++i) {
        (*this)[i] = ToUpper((*this)[i]);
    }
}

ts::UString ts::UString::toLower() const
{
    UString result(*this);
    result.convertToLower();
    return result;
}

ts::UString ts::UString::toUpper() const
{
    UString result(*this);
    result.convertToUpper();
    return result;
}


//----------------------------------------------------------------------------
// Remove all occurences of a substring.
//----------------------------------------------------------------------------

void ts::UString::remove(const UString& substr)
{
    const size_type len = substr.size();
    if (len > 0) {
        size_type index;
        while (!empty() && (index = find(substr)) != npos) {
            erase(index, len);
        }
    }
}

void ts::UString::remove(UChar c)
{
    erase(std::remove(begin(), end(), c), end());
}

ts::UString ts::UString::toRemoved(const UString& substr) const
{
    UString result(*this);
    result.remove(substr);
    return result;
}

ts::UString ts::UString::toRemoved(UChar c) const
{
    UString result(*this);
    result.remove(c);
    return result;
}


//----------------------------------------------------------------------------
// Substitute all occurences of a string with another one.
//----------------------------------------------------------------------------

void ts::UString::substitute(const UString& value, const UString& replacement)
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

ts::UString ts::UString::toSubstituted(const UString& value, const UString& replacement) const
{
    UString result(*this);
    result.substitute(value, replacement);
    return result;
}


//----------------------------------------------------------------------------
// Prefix / suffix checking.
//----------------------------------------------------------------------------

void ts::UString::removePrefix(const UString& prefix, CaseSensitivity cs)
{
    if (startWith(prefix, cs)) {
        erase(0, prefix.length());
    }
}

void ts::UString::removeSuffix(const UString& suffix, CaseSensitivity cs)
{
    if (endWith(suffix, cs)) {
        assert(length() >= suffix.length());
        erase(length() - suffix.length());
    }
}

ts::UString ts::UString::toRemovedPrefix(const UString& prefix, CaseSensitivity cs) const
{
    UString result(*this);
    result.removePrefix(prefix, cs);
    return result;
}

ts::UString ts::UString::toRemovedSuffix(const UString& suffix, CaseSensitivity cs) const
{
    UString result(*this);
    result.removeSuffix(suffix, cs);
    return result;
}

bool ts::UString::startWith(const UString& prefix, CaseSensitivity cs) const
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

bool ts::UString::endWith(const UString& suffix, CaseSensitivity cs) const
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
// Split a string into multiple lines which are not longer than a specified maximum width.
//----------------------------------------------------------------------------

ts::UString ts::UString::toSplitLines(size_type maxWidth, const ts::UString& otherSeparators, const ts::UString& nextMargin, bool forceSplit, const ts::UString lineSeparator) const
{
    UStringList lines;
    splitLines(lines, maxWidth, otherSeparators, nextMargin, forceSplit);
    return join(lines, lineSeparator);
}


//----------------------------------------------------------------------------
// Left-justify (pad and optionally truncate) string.
//----------------------------------------------------------------------------

void ts::UString::justifyLeft(size_type width, UChar pad, bool truncate)
{
    const size_type len = length();
    if (truncate && len > width) {
        erase(width);
    }
    else if (len < width) {
        append(width - len, pad);
    }
}

ts::UString ts::UString::toJustifiedLeft(size_type width, UChar pad, bool truncate) const
{
    UString result(*this);
    result.justifyLeft(width, pad, truncate);
    return result;
}


//----------------------------------------------------------------------------
// Right-justified (pad and optionally truncate) string.
//----------------------------------------------------------------------------

void ts::UString::justifyRight(size_type width, UChar pad, bool truncate)
{
    const size_type len = length();
    if (truncate && len > width) {
        erase(0, len - width);
    }
    else if (len < width) {
        insert(0, width - len, pad);
    }
}

ts::UString ts::UString::toJustifiedRight(size_type width, UChar pad, bool truncate) const
{
    UString result(*this);
    result.justifyRight(width, pad, truncate);
    return result;
}


//----------------------------------------------------------------------------
// Centered-justified (pad and optionally truncate) string.
//----------------------------------------------------------------------------

void ts::UString::justifyCentered(size_type width, UChar pad, bool truncate)
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

ts::UString ts::UString::toJustifiedCentered(size_type width, UChar pad, bool truncate) const
{
    UString result(*this);
    result.justifyCentered(width, pad, truncate);
    return result;
}


//----------------------------------------------------------------------------
// Justify string, pad in the middle.
//----------------------------------------------------------------------------

void ts::UString::justify(const UString& right, size_type width, UChar pad)
{
    const size_type len = length() + right.length();
    if (len < width) {
        append(width - len, pad);
    }
    append(right);
}

ts::UString ts::UString::toJustified(const UString& right, size_type width, UChar pad) const
{
    UString result(*this);
    result.justify(right, width, pad);
    return result;
}


//----------------------------------------------------------------------------
// Convert into a suitable HTML representation.
// For performance reasons convertToHTML() is implemented in tsUChar.cpp.
//----------------------------------------------------------------------------

ts::UString ts::UString::ToHTML() const
{
    UString result(*this);
    result.convertToHTML();
    return result;
}


//----------------------------------------------------------------------------
// Format a boolean value in various standard representation.
//----------------------------------------------------------------------------

ts::UString ts::UString::YesNo(bool b)
{
    return FromUTF8(b ? "yes" : "no");
}

ts::UString ts::UString::TrueFalse(bool b)
{
    return FromUTF8(b ? "true" : "false");
}

ts::UString ts::UString::OnOff(bool b)
{
    return FromUTF8(b ? "on" : "off");
}


//----------------------------------------------------------------------------
// Check if two strings are identical, case-insensitive and ignoring blanks
//----------------------------------------------------------------------------

bool ts::UString::similar(const UString& other) const
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

bool ts::UString::similar(const void* addr, size_type size) const
{
    return addr != 0 && similar(FromUTF8(reinterpret_cast<const char*>(addr), size));
}


//----------------------------------------------------------------------------
// Convert a DVB string into UTF-16.
//----------------------------------------------------------------------------

ts::UString ts::UString::FromDVB(const uint8_t* dvb, size_t dvbSize, const DVBCharset* charset)
{
    // Null or empty buffer is a valid empty string.
    if (dvb == 0 || dvbSize == 0) {
        return UString();
    }

    // Get the DVB character set code from the beginning of the string.
    uint32_t code = 0;
    size_t codeSize = 0;
    if (!DVBCharset::GetCharCodeTable(code, codeSize, dvb, dvbSize)) {
        return UString();
    }

    // Skip the character code.
    assert(codeSize <= dvbSize);
    dvb += codeSize;
    dvbSize -= codeSize;

    // Get the character set for this DVB string.
    if (code != 0 || charset == 0) {
        charset = DVBCharset::GetCharset(code);
    }
    if (charset == 0) {
        // Unsupported charset. Collect all ANSI characters, replace others by '.'.
        UString str(dvbSize, FULL_STOP);
        for (size_t i = 0; i < dvbSize; i++) {
            if (dvb[i] >= 0x20 && dvb[i] <= 0x7E) {
                str[i] = UChar(dvb[i]);
            }
        }
        return str;
    }
    else {
        // Convert the DVB string using the character set.
        UString str;
        charset->decode(str, dvb, dvbSize);
        return str;
    }
}


//----------------------------------------------------------------------------
// Convert a DVB string (preceded by its one-byte length) into UTF-16.
//----------------------------------------------------------------------------

ts::UString ts::UString::FromDVBWithByteLength(const uint8_t*& buffer, size_t& size, const DVBCharset* charset)
{
    // Null or empty buffer is a valid empty string.
    if (buffer == 0 || size == 0) {
        return UString();
    }

    // Address and size of the DVB string.
    const uint8_t* const dvb = buffer + 1;
    const size_t dvbSize = std::min<size_t>(buffer[0], size - 1);

    // Update the user buffer to point after the DVB string.
    buffer += dvbSize + 1;
    size -= dvbSize + 1;

    // Decode the DVB string.
    return FromDVB(dvb, dvbSize, charset);
}


//----------------------------------------------------------------------------
// Convert a UTF-16 string into DVB representation.
//----------------------------------------------------------------------------

size_t ts::UString::toDVB(uint8_t*& buffer, size_t& size, size_t start, size_t count, const DVBCharset* charset) const
{
    // Skip degenerated cases where there is nothing to do.
    if (buffer == 0 || size == 0 || start >= length()) {
        return 0;
    }

    // Try to encode using these charsets in order
    static const DVBCharset* const dvbEncoders[] = {
        &ts::DVBCharsetSingleByte::ISO_6937,     // default charset 
        &ts::DVBCharsetSingleByte::ISO_8859_15,  // most european characters and Euro currency sign
        &ts::DVBCharsetUTF8::UTF_8,              // last chance, used when no other match
        0                                        // end of list
    };

    // Look for a character set which can encode the string.
    if (charset == 0 || !charset->canEncode(*this, start, count)) {
        for (size_t i = 0; charset != 0 && dvbEncoders[i] != 0; ++i) {
            if (dvbEncoders[i]->canEncode(*this, start, count)) {
                charset = dvbEncoders[i];
            }
        }
    }
    if (charset == 0) {
        // Should not happen since UTF-8 can encode everything.
        return 0;
    }

    // Encode the string.
    return charset->encode(buffer, size, *this, start, count);
}


//----------------------------------------------------------------------------
// Convert a UTF-16 string into DVB representation in a byte block.
//----------------------------------------------------------------------------

ts::ByteBlock ts::UString::toDVB(size_t start, size_t count, const DVBCharset* charset) const
{
    if (start >= length()) {
        return ByteBlock();
    }
    else {
        // The maximum number of DVB bytes per character is 3 (worst case in UTF-8).
        ByteBlock bb(3 * std::min(length() - start, count));

        // Convert the string.
        uint8_t* buffer = bb.data();
        size_t size = bb.size();
        toDVB(buffer, size, start, count, charset);

        // Truncate unused bytes.
        assert(size <= bb.size());
        bb.resize(bb.size() - size);
        return bb;
    }
}


//----------------------------------------------------------------------------
// Convert a UTF-16 string into DVB (preceded by its one-byte length).
//----------------------------------------------------------------------------

size_t ts::UString::toDVBWithByteLength(uint8_t*& buffer, size_t& size, size_t start, size_t count, const DVBCharset* charset) const
{
    // Skip degenerated cases where there is nothing to do.
    if (buffer == 0 || size == 0 || start >= length()) {
        return 0;
    }

    // Write the DVB string at second byte, keep the first one for the length.
    uint8_t* dvbBuffer = buffer + 1;

    // We cannot write more that 255 bytes because the length must fit in one byte.
    const size_t dvbMaxSize = std::min<size_t>(size - 1, 0xFF);
    size_t dvbSize = dvbMaxSize;

    // Convert the string.
    const size_t result = toDVB(dvbBuffer, dvbSize, start, count, charset);

    // Compute the actual DVB size.
    assert(dvbSize <= dvbMaxSize);
    dvbSize = dvbMaxSize - dvbSize;

    // Update size at the beginning of the string.
    assert(dvbSize <= 0xFF);
    buffer[0] = uint8_t(dvbSize);

    // Update user's buffer characteristics.
    assert(size >= dvbSize + 1);
    buffer += dvbSize + 1;
    size -= dvbSize + 1;

    return result;
}
