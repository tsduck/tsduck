//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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

// In the implementation of UString, we allow the implicit UTF-8 conversions
// to ensure that the symbols are defined and exported from the DLL on Windows.
// Otherwise, applications may get undefined symbols if they allow these
// implicit conversions.
#define TS_ALLOW_IMPLICIT_UTF8_CONVERSION 1

#include "tsUString.h"
#include "tsByteBlock.h"
#include "tsSysUtils.h"
#include "tsDVBCharsetSingleByte.h"
#include "tsDVBCharsetUTF8.h"
TSDUCK_SOURCE;

// The UTF-8 Byte Order Mark
const char* const ts::UString::UTF8_BOM = "\xEF\xBB\xBF";

// Default separator string for groups of thousands, a comma.
const ts::UString ts::UString::DEFAULT_THOUSANDS_SEPARATOR(1, u',');

//! A reference empty string.
const ts::UString ts::UString::EMPTY;


//----------------------------------------------------------------------------
// General routine to convert from UTF-16 to UTF-8.
//----------------------------------------------------------------------------

void ts::UString::ConvertUTF16ToUTF8(const UChar*& inStart, const UChar* inEnd, char*& outStart, char* outEnd)
{
    uint32_t code;
    uint32_t high6;

    while (inStart < inEnd && outStart < outEnd) {

        // Get current code point as 16-bit value.
        code = *inStart++;

        // Get the higher 6 bits of the 16-bit value.
        high6 = code & 0xFC00;

        // The possible ranges are:
        // - 0x0000-0x0xD7FF : direct 16-bit code point.
        // - 0xD800-0x0xDBFF : leading surrogate, first part of a surrogate pair.
        // - 0xDC00-0x0xDFFF : trailing surrogate, second part of a surrogate pair,
        //                     invalid and ignored if encountered as first value.
        // - 0xE000-0x0xFFFF : direct 16-bit code point.

        if (high6 == 0xD800) {
            // This is a "leading surrogate", must be followed by a "trailing surrogate".
            if (inStart >= inEnd) {
                // Invalid truncated input string, stop here.
                break;
            }
            // A surrogate pair always gives a code point value over 0x10000.
            // This will be encoded in UTF-8 using 4 bytes, check that we have room for it.
            if (outStart + 4 > outEnd) {
                inStart--;  // Push back the leading surrogate into the input buffer.
                break;
            }
            // Get the "trailing surrogate".
            const uint32_t surr = *inStart++;
            // Ignore the code point if the leading surrogate is not in the valid range.
            if ((surr & 0xFC00) == 0xDC00) {
                // Rebuild the 32-bit value of the code point.
                code = 0x010000 + (((code - 0xD800) << 10) | (surr - 0xDC00));
                // Encode it as 4 bytes in UTF-8.
                outStart[3] = char(0x80 | (code & 0x3F));
                code >>= 6;
                outStart[2] = char(0x80 | (code & 0x3F));
                code >>= 6;
                outStart[1] = char(0x80 | (code & 0x3F));
                code >>= 6;
                outStart[0] = char(0xF0 | (code & 0x07));
                outStart += 4;
            }
        }

        else if (high6 != 0xDC00) {
            // The 16-bit value is the code point.
            if (code < 0x0080) {
                // ASCII compatible value, one byte encoding.
                *outStart++ = char(code);
            }
            else if (code < 0x800 && outStart + 1 < outEnd) {
                // 2 bytes encoding.
                outStart[1] = char(0x80 | (code & 0x3F));
                code >>= 6;
                outStart[0] = char(0xC0 | (code & 0x1F));
                outStart += 2;
            }
            else if (code >= 0x800 && outStart + 2 < outEnd) {
                // 3 bytes encoding.
                outStart[2] = char(0x80 | (code & 0x3F));
                code >>= 6;
                outStart[1] = char(0x80 | (code & 0x3F));
                code >>= 6;
                outStart[0] = char(0xE0 | (code & 0x0F));
                outStart += 3;
            }
            else {
                // There not enough space in the output buffer.
                inStart--;  // Push back the leading surrogate into the input buffer.
                break;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Output operator for ts::UChar on standard text streams with UTF-8 conv.
//----------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& strm, const ts::UChar c)
{
    // See comments in ConvertUTF16ToUTF8().
    if ((c & 0xF800) == 0xD800) {
        // Part of a surrogate pair, cannot be displayed alone.
        return strm;
    }
    else if (c < 0x0080) {
        // ASCII compatible value, one byte encoding.
        return strm << char(c);
    }
    else if (c < 0x0800) {
        // 2 bytes encoding.
        return strm << char(0x80 | (c & 0x3F)) << char(0xC0 | (c & 0x1F));
    }
    else {
        // 3 bytes encoding.
        return strm << char(0x80 | (c & 0x3F)) << char(0x80 | (c & 0x3F)) << char(0xE0 | (c & 0x0F));
    }
}


//----------------------------------------------------------------------------
// General routine to convert from UTF-8 to UTF-16.
//----------------------------------------------------------------------------

void ts::UString::ConvertUTF8ToUTF16(const char*& inStart, const char* inEnd, UChar*& outStart, UChar* outEnd)
{
    uint32_t code;

    while (inStart < inEnd && outStart < outEnd) {

        // Get current code point at 8-bit value.
        code = *inStart++ & 0xFF;

        // Process potential continuation bytes and rebuild the code point.
        // Note: to speed up the processing, we do not check that continuation bytes,
        // if any, match the binary pattern 10xxxxxx.

        if (code < 0x80) {
            // 0xxx xxxx, ASCII compatible value, one byte encoding.
            *outStart++ = uint16_t(code);
        }
        else if ((code & 0xE0) == 0xC0) {
            // 110x xxx, 2 byte encoding.
            if (inStart >= inEnd) {
                // Invalid truncated input string, stop here.
                break;
            }
            else {
                *outStart++ = uint16_t((code & 0x1F) << 6) | (*inStart++ & 0x3F);
            }
        }
        else if ((code & 0xF0) == 0xE0) {
            // 1110 xxxx, 3 byte encoding.
            if (inStart + 1 >= inEnd) {
                // Invalid truncated input string, stop here.
                inStart = inEnd;
                break;
            }
            else {
                *outStart++ = uint16_t((code & 0x0F) << 12) | ((uint16_t(inStart[0] & 0x3F)) << 6) | (inStart[1] & 0x3F);
                inStart += 2;
            }
        }
        else if ((code & 0xF8) == 0xF0) {
            // 1111 0xxx, 4 byte encoding.
            if (inStart + 2 >= inEnd) {
                // Invalid truncated input string, stop here.
                inStart = inEnd;
                break;
            }
            else if (outStart + 1 >= outEnd) {
                // We need 2 16-bit values in UTF-16.
                inStart--;  // Push back the leading byte into the input buffer.
                break;
            }
            else {
                code = ((code & 0x07) << 18) | ((uint32_t(inStart[0] & 0x3F)) << 12) | ((uint32_t(inStart[1] & 0x3F)) << 6) | (inStart[2] & 0x3F);
                inStart += 3;
                code -= 0x10000;
                *outStart++ = uint16_t(0xD800 + (code >> 10));
                *outStart++ = uint16_t(0xDC00 + (code & 0x03FF));
            }
        }
        else {
            // 10xx xxxx, continuation byte, invalid here, simply ignore it.
            // 1111 1xxx, an invalid UTF-8 value, ignore as well.
            assert((code & 0xC0) == 0x80 || (code & 0xF8) == 0xF8);
        }
    }
}


//----------------------------------------------------------------------------
// Append a Unicode code point into the string.
//----------------------------------------------------------------------------

ts::UString& ts::UString::append(uint32_t code)
{
    if (code <= 0xD7FF || (code >= 0xE000 && code <= 0xFFFF)) {
        // One single 16-bit value.
        push_back(UChar(code));
    }
    else if (code >= 0x00010000 && code <= 0x0010FFFF) {
        // A surrogate pair.
        code -= 0x00010000;
        push_back(UChar(0xD800 + (code >> 10)));
        push_back(UChar(0xDC00 + (code & 0x03FF)));
    }
    return *this;
}


//----------------------------------------------------------------------------
// Convert an UTF-8 string into a new UString.
//----------------------------------------------------------------------------

ts::UString ts::UString::FromUTF8(const std::string& utf8)
{
    UString str;
    str.assignFromUTF8(utf8);
    return str;
}

ts::UString ts::UString::FromUTF8(const char* utf8)
{
    UString str;
    str.assignFromUTF8(utf8);
    return str;
}

ts::UString ts::UString::FromUTF8(const char* utf8, size_type count)
{
    UString str;
    str.assignFromUTF8(utf8, count);
    return str;
}


//----------------------------------------------------------------------------
// Convert an UTF-8 string into this object.
//----------------------------------------------------------------------------

ts::UString& ts::UString::assignFromUTF8(const char* utf8)
{
    return assignFromUTF8(utf8, utf8 == nullptr ? 0 : ::strlen(utf8));
}

ts::UString& ts::UString::assignFromUTF8(const char* utf8, size_type count)
{
    if (utf8 == nullptr) {
        clear();
    }
    else {
        // Resize the string over the maximum size.
        // The number of UTF-16 codes is always less than the number of UTF-8 bytes.
        resize(count);

        // Convert from UTF-8 directly into this object.
        const char* inStart = utf8;
        UChar* outStart = const_cast<UChar*>(data());
        ConvertUTF8ToUTF16(inStart, inStart + count, outStart, outStart + size());

        assert(inStart >= utf8);
        assert(inStart == utf8 + count);
        assert(outStart >= data());
        assert(outStart <= data() + size());

        // Truncate to the exact number of characters.
        resize(outStart - data());
    }
    return *this;
}


//----------------------------------------------------------------------------
// Convert this UTF-16 string into UTF-8.
//----------------------------------------------------------------------------

void ts::UString::toUTF8(std::string& utf8) const
{
    // The maximum number of UTF-8 bytes is 1.5 times the number of UTF-16 codes.
    utf8.resize(2 * size());

    const UChar* inStart = data();
    char* outStart = const_cast<char*>(utf8.data());
    ConvertUTF16ToUTF8(inStart, inStart + size(), outStart, outStart + utf8.size());

    utf8.resize(outStart - utf8.data());
}

std::string ts::UString::toUTF8() const
{
    std::string utf8;
    toUTF8(utf8);
    return utf8;
}


//----------------------------------------------------------------------------
// Output operator for ts::UString on standard text streams with UTF-8 conv.
//----------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& strm, const ts::UString& str)
{
    std::string utf8;
    str.toUTF8(utf8);
    return strm << utf8;
}

std::ostream& operator<<(std::ostream& strm, const ts::UChar* str)
{
    std::string utf8;
    ts::UString(str).toUTF8(utf8);
    return strm << utf8;
}


//----------------------------------------------------------------------------
// Check if a character uses no space on display.
//----------------------------------------------------------------------------

namespace {
    inline bool NoSpace(ts::UChar c)
    {
        return ts::IsCombiningDiacritical(c) || ts::IsTrailingSurrogate(c);
    }
}


//----------------------------------------------------------------------------
// Get the display width in characters.
//----------------------------------------------------------------------------

ts::UString::size_type ts::UString::width() const
{
    if (empty()) {
        return 0;
    }
    else {
        // Ignore all combining diacritical and trailing surrogate characters and after the first one.
        // A diacritical character in first position does count since it cannot be combined with the previous one.
        // We do not check that surrogate pairs are correctly formed, we just skip trailing ones.
        size_type wid = 1;
        for (const UChar* p = data() + 1; p < last(); ++p) {
            if (!NoSpace(*p)) {
                ++wid;
            }
        }
        return wid;
    }
}


//----------------------------------------------------------------------------
// Count displayed positions inside a string.
//----------------------------------------------------------------------------

ts::UString::size_type ts::UString::displayPosition(size_type count, size_type from, StringDirection direction) const
{
    const UChar* const base = data();
    switch (direction) {
        case LEFT_TO_RIGHT: {
            // Move forward.
            while (from < size() && count > 0) {
                if (!NoSpace(base[from])) {
                    --count;
                }
                ++from;
            }
            // Move after combining sequence.
            while (from < size() && NoSpace(base[from])) {
                ++from;
            }
            return std::min(from, size());
        }
        case RIGHT_TO_LEFT: {
            // Start at end of string, at worst.
            from = std::min(from, size());
            // Move backward.
            while (from > 0 && count > 0) {
                --from;
                if (!NoSpace(base[from])) {
                    --count;
                }
            }
            // Move at start of combining sequence.
            while (from > 0 && NoSpace(base[from])) {
                --from;
            }
            return from;
        }
        default: {
            // Should not get there.
            assert(false);
            return size();
        }
    }
}


//----------------------------------------------------------------------------
// Truncate this string to a given display width.
//----------------------------------------------------------------------------

void ts::UString::truncateWidth(size_type maxWidth, StringDirection direction)
{
    switch (direction) {
        case LEFT_TO_RIGHT: {
            const size_t pos = displayPosition(maxWidth, 0, LEFT_TO_RIGHT);
            resize(pos);
            break;
        }
        case RIGHT_TO_LEFT: {
            const size_t pos = displayPosition(maxWidth, length(), RIGHT_TO_LEFT);
            erase(0, pos);
            break;
        }
        default: {
            // Should not get there.
            assert(false);
        }
    }
}

ts::UString ts::UString::toTruncatedWidth(size_type maxWidth, StringDirection direction) const
{
    UString result(*this);
    result.truncateWidth(maxWidth, direction);
    return result;
}


//----------------------------------------------------------------------------
// Reverse the order of characters in the string.
//----------------------------------------------------------------------------

void ts::UString::reverse()
{
    std::reverse(begin(), end());
}

ts::UString ts::UString::toReversed() const
{
    UString result(*this);
    result.reverse();
    return result;
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
        size_type index = 0;
        while (!empty() && (index = find(substr, index)) != NPOS) {
            erase(index, len);
        }
    }
}

void ts::UString::remove(UChar c)
{
    size_type index = 0;
    while (!empty() && (index = find(c, index)) != NPOS) {
        erase(index, 1);
    }
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

bool ts::UString::contain(const UString& substring, CaseSensitivity cs) const
{
    switch (cs) {
        case CASE_SENSITIVE: {
            return find(substring) != NPOS;
        }
        case CASE_INSENSITIVE: {
            return toLower().find(substring.toLower()) != NPOS;
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
    return Join(lines, lineSeparator);
}


//----------------------------------------------------------------------------
// Left-justify (pad and optionally truncate) string.
//----------------------------------------------------------------------------

void ts::UString::justifyLeft(size_type wid, UChar pad, bool truncate, size_t spacesBeforePad)
{
    const size_type len = width();
    if (truncate && len > wid) {
        truncateWidth(wid);
    }
    else if (len < wid) {
        spacesBeforePad = std::min(spacesBeforePad, wid - len);
        append(spacesBeforePad, SPACE);
        append(wid - len - spacesBeforePad, pad);
    }
}

ts::UString ts::UString::toJustifiedLeft(size_type wid, UChar pad, bool truncate, size_t spacesBeforePad) const
{
    UString result(*this);
    result.justifyLeft(wid, pad, truncate, spacesBeforePad);
    return result;
}


//----------------------------------------------------------------------------
// Right-justified (pad and optionally truncate) string.
//----------------------------------------------------------------------------

void ts::UString::justifyRight(size_type wid, UChar pad, bool truncate, size_t spacesAfterPad)
{
    const size_type len = width();
    if (truncate && len > wid) {
        truncateWidth(wid, RIGHT_TO_LEFT);
    }
    else if (len < wid) {
        spacesAfterPad = std::min(spacesAfterPad, wid - len);
        insert(0, spacesAfterPad, SPACE);
        insert(0, wid - len - spacesAfterPad, pad);
    }
}

ts::UString ts::UString::toJustifiedRight(size_type wid, UChar pad, bool truncate, size_t spacesAfterPad) const
{
    UString result(*this);
    result.justifyRight(wid, pad, truncate, spacesAfterPad);
    return result;
}


//----------------------------------------------------------------------------
// Centered-justified (pad and optionally truncate) string.
//----------------------------------------------------------------------------

void ts::UString::justifyCentered(size_type wid, UChar pad, bool truncate, size_t spacesAroundPad)
{
    const size_type len = width();
    if (truncate && len > wid) {
        truncateWidth(wid);
    }
    else if (len < wid) {
        const size_type leftSize = (wid - len) / 2;
        const size_type leftSpaces = std::min(spacesAroundPad, leftSize);
        const size_type rightSize = wid - len - leftSize;
        const size_type rightSpaces = std::min(spacesAroundPad, rightSize);
        insert(0, leftSpaces, SPACE);
        insert(0, leftSize - leftSpaces, pad);
        append(rightSpaces, SPACE);
        append(rightSize - rightSpaces, pad);
    }
}

ts::UString ts::UString::toJustifiedCentered(size_type wid, UChar pad, bool truncate, size_t spacesAroundPad) const
{
    UString result(*this);
    result.justifyCentered(wid, pad, truncate, spacesAroundPad);
    return result;
}


//----------------------------------------------------------------------------
// Justify string, pad in the middle.
//----------------------------------------------------------------------------

void ts::UString::justify(const UString& right, size_type wid, UChar pad, size_t spacesAroundPad)
{
    const size_type len = this->width() + right.width();
    if (len < wid) {
        const size_t padWidth = wid - len;
        const size_t leftSpaces = std::min(spacesAroundPad, padWidth);
        const size_t rightSpaces = std::min(spacesAroundPad, padWidth - leftSpaces);
        append(leftSpaces, SPACE);
        append(padWidth - rightSpaces - leftSpaces, pad);
        append(rightSpaces, SPACE);
    }
    append(right);
}

ts::UString ts::UString::toJustified(const UString& right, size_type wid, UChar pad, size_t spacesAroundPad) const
{
    UString result(*this);
    result.justify(right, wid, pad, spacesAroundPad);
    return result;
}


//----------------------------------------------------------------------------
// Convert HTML representation. For performance reasons convertToHTML() and
// convertFromHTML() are implemented in tsUChar.cpp.
//----------------------------------------------------------------------------

ts::UString ts::UString::toHTML(const UString& convert) const
{
    UString result(*this);
    result.convertToHTML(convert);
    return result;
}

ts::UString ts::UString::fromHTML() const
{
    UString result(*this);
    result.convertFromHTML();
    return result;
}


//----------------------------------------------------------------------------
// Convert JSON representations.
//----------------------------------------------------------------------------

ts::UString ts::UString::toJSON() const
{
    UString result(*this);
    result.convertToJSON();
    return result;
}

ts::UString ts::UString::fromJSON() const
{
    UString result(*this);
    result.convertFromJSON();
    return result;
}

void ts::UString::convertToJSON()
{
    for (size_type i = 0; i < length(); ) {
        const UChar c = at(i);

        // Known backslash sequences.
        UChar quoted = CHAR_NULL;
        switch (c) {
            case QUOTATION_MARK:
            case REVERSE_SOLIDUS: quoted = c; break;
            case BACKSPACE: quoted = u'b'; break;
            case FORM_FEED: quoted = u'f'; break;
            case LINE_FEED: quoted = u'n'; break;
            case CARRIAGE_RETURN: quoted = u'r'; break;
            case HORIZONTAL_TABULATION: quoted = u't'; break;
            default: break;
        }

        if (quoted != CHAR_NULL) {
            // Single character backslash sequence
            at(i) = REVERSE_SOLIDUS;
            insert(i + 1, 1, quoted);
            i += 2;
        }
        else if (c >= 0x0020 && c <= 0x007E) {
            // Unmodified character
            i++;
        }
        else {
            // Other Unicode character, use hex code.
            at(i) = REVERSE_SOLIDUS;
            insert(i + 1, Format(u"u%04X", {uint16_t(c)}));
            i += 6;
        }
    }
}

void ts::UString::convertFromJSON()
{
    // We don't check the last character (a final backslash cannot be modified).
    if (length() > 1) {
        for (size_type i = 0; i < length() - 1; ++i) {
            if (at(i) == REVERSE_SOLIDUS) {
                const UChar c = at(i+1);
                UChar unquoted = CHAR_NULL;
                if (c == u'u' && i+6 <= length() && (u"0x" + substr(i+2, 4)).toInteger(unquoted)) {
                    // Hexa sequence.
                    at(i) = unquoted;
                    erase(i+1, 5);
                }
                else {
                    // Single character sequence.
                    switch (c) {
                        case QUOTATION_MARK:
                        case REVERSE_SOLIDUS:
                        case SOLIDUS: unquoted = c; break;
                        case u'b': unquoted = BACKSPACE; break;
                        case u'f': unquoted = FORM_FEED; break;
                        case u'n': unquoted = LINE_FEED; break;
                        case u'r': unquoted = CARRIAGE_RETURN; break;
                        case u't': unquoted = HORIZONTAL_TABULATION; break;
                        default: break;
                    }
                    if (unquoted != CHAR_NULL) {
                        at(i) = unquoted;
                        erase(i+1, 1);
                    }
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Various specialized messages.
//----------------------------------------------------------------------------

ts::UString ts::UString::YesNo(bool b)
{
    return b ? u"yes" : u"no";
}

ts::UString ts::UString::TrueFalse(bool b)
{
    return b ? u"true" : u"false";
}

ts::UString ts::UString::OnOff(bool b)
{
    return b ? u"on" : u"off";
}

ts::UString ts::UString::TristateYesNo(Tristate b)
{
    return int(b) < 0 ? u"maybe" : YesNo(bool(b));
}

ts::UString ts::UString::TristateTrueFalse(Tristate b)
{
    return int(b) < 0 ? u"unknown" : TrueFalse(bool(b));
}

ts::UString ts::UString::TristateOnOff(Tristate b)
{
    return int(b) < 0 ? u"unknown" : OnOff(bool(b));
}

ts::UString ts::UString::AfterBytes(const std::streampos& position)
{
    const int64_t bytes = int64_t(position);
    return bytes <= 0 ? UString() : Format(u" after %'d bytes", {bytes});
}

ts::UString ts::UString::HumanSize(int64_t value, const UString& units, bool forceSign)
{
    const int64_t k = TS_CONST64(1024);

    if (value < 8 * k) { // less than 8 kB => use bytes
        return Decimal(value, 0, true, u",", forceSign) + u" " + units;
    }
    else if (value < 8 * k * k) { // between 8 kB and 8 MB => use kB
        return Decimal(value / k, 0, true, u",", forceSign) + u" k" + units;
    }
    else if (value < 8 * k * k * k) { // between 8 MB and 8 GB => use MB
        return Decimal(value / (k * k), 0, true, u",", forceSign) + u" M" + units;
    }
    else { // more than 8 GB => use GB
        return Decimal(value / (k * k * k), 0, true, u",", forceSign) + u" G" + units;
    }
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
    return addr != nullptr && similar(FromUTF8(reinterpret_cast<const char*>(addr), size));
}


//----------------------------------------------------------------------------
// Read one UTF-8 line from a text file and load it into this object.
//----------------------------------------------------------------------------

bool ts::UString::getLine(std::istream& strm)
{
    std::string line;

    if (!std::getline(strm, line)) {
        // File read error.
        clear();
        return false;
    }
    else {
        const char* start = line.data();
        size_type len = line.size();

        // Remove potential trailing mixed CR/LF characters
        while (len > 0 && (start[len - 1] == '\r' || start[len - 1] == '\n')) {
            --len;
        }

        // Remove potential UTF-8 BOM (Byte Order Mark) at beginning of line.
        if (len >= UTF8_BOM_SIZE && line.compare(0, UTF8_BOM_SIZE, UTF8_BOM, UTF8_BOM_SIZE) == 0) {
            start += UTF8_BOM_SIZE;
            len -= UTF8_BOM_SIZE;
        }

        // Convert from UTF-8 to UTF-16.
        assignFromUTF8(start, len);
        return true;
    }
}


//----------------------------------------------------------------------------
// Convert a string into a bool value.
//----------------------------------------------------------------------------

namespace {
    const ts::Enumeration BoolEnum({
        {u"false",   0},
        {u"true",    1},
        {u"yes",     1},
        {u"no",      0},
        {u"on",      1},
        {u"off",     0},
    });
}

bool ts::UString::toBool(bool& value) const
{
    const int iValue = BoolEnum.value(*this, false);

    if (iValue == Enumeration::UNKNOWN) {
        // Invalid string and invalid integer.
        value = false;
        return false;
    }
    else {
        value = iValue != 0;
        return true;
    }
}


//----------------------------------------------------------------------------
// Convert a string into a Tristate value.
//----------------------------------------------------------------------------

// An enumeration for Tristate values. We use very large integer values
// for predefined strings to avoid clash with user-specified values.
namespace {
    enum {
        TSE_FALSE = std::numeric_limits<int>::min(),
        TSE_TRUE,
        TSE_YES,
        TSE_NO,
        TSE_ON,
        TSE_OFF,
        TSE_MAYBE,
        TSE_UNKNOWN,
        TSE_LAST  // Last predefined value
    };
    const ts::Enumeration TristateEnum({
        {u"false",   TSE_FALSE},
        {u"true",    TSE_TRUE},
        {u"yes",     TSE_YES},
        {u"no",      TSE_NO},
        {u"on",      TSE_ON},
        {u"off",     TSE_OFF},
        {u"maybe",   TSE_MAYBE},
        {u"unknown", TSE_UNKNOWN},
    });
}

ts::UString ts::UString::TristateNamesList()
{
    return TristateEnum.nameList();
}

bool ts::UString::toTristate(Tristate& value) const
{
    const int iValue = TristateEnum.value(*this, false);

    if (iValue == Enumeration::UNKNOWN) {
        // Invalid string and invalid integer.
        value = MAYBE;
        return false;
    }
    else {
        // Valid string or integer.
        switch (iValue) {
            case TSE_FALSE:
            case TSE_NO:
            case TSE_OFF:
                value = FALSE;
                break;
            case TSE_TRUE:
            case TSE_YES:
            case TSE_ON:
                value = TRUE;
                break;
            case TSE_MAYBE:
            case TSE_UNKNOWN:
                value = MAYBE;
                break;
            default:
                // Got an integer value.
                value = ToTristate(iValue);
                break;
        }
        return true;
    }
}


//----------------------------------------------------------------------------
// Interpret this string as a sequence of hexadecimal digits (ignore blanks).
//----------------------------------------------------------------------------

bool ts::UString::hexaDecode(ts::ByteBlock& result) const
{
    result.clear();
    return hexaDecodeAppend(result);
}

bool ts::UString::hexaDecodeAppend(ts::ByteBlock& result) const
{
    // Oversize the prereservation in output buffer.
    result.reserve(result.size() + size() / 2);

    bool got_first_nibble = false;
    uint8_t byte = 0;
    uint8_t nibble = 0;

    for (const UChar* p = data(); p < last(); ++p) {
        if (IsSpace(*p)) {
            // Ignore spaces.
            continue;
        }
        else if ((nibble = uint8_t(ToDigit(*p, 16, 0xFF))) == 0xFF) {
            // Invalid hexa digit.
            return false;
        }
        if (got_first_nibble) {
            result.push_back(byte | nibble);
        }
        else {
            byte = nibble << 4;
        }
        got_first_nibble = !got_first_nibble;
    }

    return !got_first_nibble;
}


//----------------------------------------------------------------------------
// Build a multi-line string containing the hexadecimal dump of a memory area.
// Wrapper methods.
//----------------------------------------------------------------------------

ts::UString ts::UString::Dump(const void *data,
                              size_type size,
                              uint32_t flags,
                              size_type indent,
                              size_type line_width,
                              size_type init_offset,
                              size_type inner_indent)
{
    UString s;
    s.appendDump(data, size, flags, indent, line_width, init_offset, inner_indent);
    return s;
}

ts::UString ts::UString::Dump(const ByteBlock& bb,
                              uint32_t flags,
                              size_type indent,
                              size_type line_width,
                              size_type init_offset,
                              size_type inner_indent)
{
    UString s;
    s.appendDump(bb.data(), bb.size(), flags, indent, line_width, init_offset, inner_indent);
    return s;
}

void ts::UString::appendDump(const ByteBlock& bb,
                             uint32_t flags,
                             size_type indent,
                             size_type line_width,
                             size_type init_offset,
                             size_type inner_indent)
{
    appendDump(bb.data(), bb.size(), flags, indent, line_width, init_offset, inner_indent);
}


//----------------------------------------------------------------------------
// Build a multi-line string containing the hexadecimal dump of a memory area.
//----------------------------------------------------------------------------

void ts::UString::appendDump(const void *data,
                             size_type size,
                             uint32_t flags,
                             size_type indent,
                             size_type line_width,
                             size_type init_offset,
                             size_type inner_indent)
{
    const uint8_t* raw = static_cast<const uint8_t*>(data);

    // Make sure we have something to display (default is hexa)
    if ((flags & (HEXA | C_STYLE | BINARY | BIN_NIBBLE | ASCII)) == 0) {
        flags |= HEXA;
    }
    if ((flags & COMPACT) != 0) {
        // COMPACT implies SINGLE_LINE.
        flags |= SINGLE_LINE;
    }

    // Width of an hexa byte: "XX" (2) or "0xXX," (5)
    size_type hexa_width = 0;
    UString byte_prefix;
    UString byte_suffix;

    if (flags & C_STYLE) {
        hexa_width  = 5;
        byte_prefix = u"0x";
        byte_suffix = u",";
        flags |= HEXA; // Enforce hexa flag
    }
    else if (flags & (HEXA | SINGLE_LINE)) {
        hexa_width  = 2;
    }
    else {
        hexa_width  = 0;
    }

    // Specific case: simple dump, everything on one line.
    if (flags & SINGLE_LINE) {
        reserve(length() + (hexa_width + 1) * size);
        for (size_type i = 0; i < size; ++i) {
            if (i > 0 && (flags & COMPACT) == 0) {
                append(1, u' ');
            }
            append(byte_prefix);
            append(Hexa(raw[i], 0, UString(), false, true));
            append(byte_suffix);
        }
        return;
    }

    // Width of offset field
    size_type offset_width;

    if ((flags & OFFSET) == 0) {
        offset_width = 0;
    }
    else if (flags & WIDE_OFFSET) {
        offset_width = 8;
    }
    else if (init_offset + size <= 0x10000) {
        offset_width = 4;
    }
    else {
        offset_width = 8;
    }

    // Width of a binary byte
    size_type bin_width;

    if (flags & BIN_NIBBLE) {
        bin_width = 9;
        flags |= BINARY;  // Enforce binary flag
    }
    else if (flags & BINARY) {
        bin_width = 8;
    }
    else {
        bin_width = 0;
    }

    // Pre-allocation to avoid too frequent reallocations.
    reserve(length() + indent + inner_indent + (hexa_width + bin_width + 5) * size);

    // Number of non-byte characters
    size_type add_width = indent + inner_indent;
    if (offset_width != 0) {
        add_width += offset_width + 3;
    }
    if ((flags & HEXA) && (flags & (BINARY | ASCII))) {
        add_width += 2;
    }
    if ((flags & BINARY) && (flags & ASCII)) {
        add_width += 2;
    }

    // Computes max number of dumped bytes per line
    size_type bytes_per_line;

    if (flags & BPL) {
        bytes_per_line = line_width;
    }
    else if (add_width >= line_width) {
        bytes_per_line = 8;  // arbitrary, if indent is too long
    }
    else {
        bytes_per_line = (line_width - add_width) /
            (((flags & HEXA) ? (hexa_width + 1) : 0) +
             ((flags & BINARY) ? (bin_width + 1) : 0) +
             ((flags & ASCII) ? 1 : 0));
        if (bytes_per_line > 1) {
            bytes_per_line = bytes_per_line & ~1; // force even value
        }
    }
    if (bytes_per_line == 0) {
        bytes_per_line = 8;  // arbitrary, if ended up with none
    }

    // Display data
    for (size_type line = 0; line < size; line += bytes_per_line) {

        // Number of bytes on this line (last line may be shorter)
        size_type line_size = line + bytes_per_line <= size ? bytes_per_line : size - line;

        // Beginning of line
        append(indent, u' ');
        if (flags & OFFSET) {
            append(Hexa(init_offset + line, offset_width, UString(), false, true));
            append(u":  ");
        }
        append(inner_indent, u' ');

        // Hexa dump
        if (flags & HEXA) {
            for (size_type byte = 0; byte < line_size; byte++) {
                append(byte_prefix);
                append(Hexa(raw[line + byte], 0, UString(), false, true));
                append(byte_suffix);
                if (byte < bytes_per_line - 1) {
                    append(1, u' ');
                }
            }
            if (flags & (BINARY | ASCII)) { // more to come
                if (line_size < bytes_per_line) {
                    append((hexa_width + 1) * (bytes_per_line - line_size) - 1, u' ');
                }
                append(2, u' ');
            }
        }

        // Binary dump
        if (flags & BINARY) {
            for (size_type byte = 0; byte < line_size; byte++) {
                int b = int(raw[line + byte]);
                for (int i = 7; i >= 0; i--) {
                    append(1, UChar(u'0' + ((b >> i) & 0x01)));
                    if (i == 4 && (flags & BIN_NIBBLE) != 0) {
                        append(1, u'.');
                    }
                }
                if (byte < bytes_per_line - 1) {
                    append(1, u' ');
                }
            }
            if (flags & ASCII) { // more to come
                if (line_size < bytes_per_line) {
                    append((bin_width + 1) * (bytes_per_line - line_size) - 1, u' ');
                }
                append(2, u' ');
            }
        }

        // ASCII dump
        if (flags & ASCII) {
            for (size_type byte = 0; byte < line_size; byte++) {
                // Display only ASCII characters. Other encodings don't make sense on one bytes.
                const UChar c = UChar(raw[line + byte]);
                push_back(c >= 0x20 && c <= 0x7E ? c : u'.');
            }
        }

        // Insert a new-line, cleanup spurious spaces.
        while (!empty() && back() == u' ') {
            pop_back();
        }
        push_back(u'\n');
    }
}


//----------------------------------------------------------------------------
// Convert a DVB string into UTF-16.
//----------------------------------------------------------------------------

ts::UString ts::UString::FromDVB(const uint8_t* dvb, size_type dvbSize, const DVBCharset* charset)
{
    // Null or empty buffer is a valid empty string.
    if (dvb == nullptr || dvbSize == 0) {
        return UString();
    }

    // Get the DVB character set code from the beginning of the string.
    uint32_t code = 0;
    size_type codeSize = 0;
    if (!DVBCharset::GetCharCodeTable(code, codeSize, dvb, dvbSize)) {
        return UString();
    }

    // Skip the character code.
    assert(codeSize <= dvbSize);
    dvb += codeSize;
    dvbSize -= codeSize;

    // Get the character set for this DVB string.
    if (code != 0 || charset == nullptr) {
        charset = DVBCharset::GetCharset(code);
    }
    if (charset == nullptr) {
        // Unsupported charset. Collect all ANSI characters, replace others by '.'.
        UString str(dvbSize, FULL_STOP);
        for (size_type i = 0; i < dvbSize; i++) {
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
    if (buffer == nullptr || size == 0) {
        return UString();
    }

    // Address and size of the DVB string.
    const uint8_t* const dvb = buffer + 1;
    const size_type dvbSize = std::min<size_t>(buffer[0], size - 1);

    // Update the user buffer to point after the DVB string.
    buffer += dvbSize + 1;
    size -= dvbSize + 1;

    // Decode the DVB string.
    return FromDVB(dvb, dvbSize, charset);
}


//----------------------------------------------------------------------------
// Convert a UTF-16 string into DVB representation.
//----------------------------------------------------------------------------

ts::UString::size_type ts::UString::toDVB(uint8_t*& buffer, size_t& size, size_type start, size_type count, const DVBCharset* charset) const
{
    // Skip degenerated cases where there is nothing to do.
    if (buffer == nullptr || size == 0 || start >= length()) {
        return 0;
    }

    // Try to encode using these charsets in order
    static const DVBCharset* const dvbEncoders[] = {
        &ts::DVBCharsetSingleByte::ISO_6937,     // default charset
        &ts::DVBCharsetSingleByte::ISO_8859_15,  // most european characters and Euro currency sign
        &ts::DVBCharsetUTF8::UTF_8,              // last chance, used when no other match
        nullptr                                  // end of list
    };

    // Look for a character set which can encode the string.
    if (charset == nullptr || !charset->canEncode(*this, start, count)) {
        for (size_type i = 0; dvbEncoders[i] != nullptr; ++i) {
            if (dvbEncoders[i]->canEncode(*this, start, count)) {
                charset = dvbEncoders[i];
                break;
            }
        }
    }
    if (charset == nullptr) {
        // Should not happen since UTF-8 can encode everything.
        return 0;
    }

    // Serialize the table code.
    const size_t codeSize = charset->encodeTableCode(buffer, size);

    // Encode the string.
    return codeSize + charset->encode(buffer, size, *this, start, count);
}


//----------------------------------------------------------------------------
// Convert a UTF-16 string into DVB representation in a byte block.
//----------------------------------------------------------------------------

ts::ByteBlock ts::UString::toDVB(size_type start, size_type count, const DVBCharset* charset) const
{
    if (start >= length()) {
        return ByteBlock();
    }
    else {
        // The maximum number of DVB bytes per character is 4 (worst case in UTF-8).
        ByteBlock bb(UTF8_CHAR_MAX_SIZE * std::min(length() - start, count));

        // Convert the string.
        uint8_t* buffer = bb.data();
        size_type size = bb.size();
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

ts::UString::size_type ts::UString::toDVBWithByteLength(uint8_t*& buffer, size_t& size, size_type start, size_type count, const DVBCharset* charset) const
{
    // Skip degenerated cases where there is nothing to do.
    if (buffer == nullptr || size == 0 || start >= length()) {
        return 0;
    }

    // Write the DVB string at second byte, keep the first one for the length.
    uint8_t* dvbBuffer = buffer + 1;

    // We cannot write more that 255 bytes because the length must fit in one byte.
    const size_type dvbMaxSize = std::min<size_t>(size - 1, 0xFF);
    size_type dvbSize = dvbMaxSize;

    // Convert the string.
    const size_type result = toDVB(dvbBuffer, dvbSize, start, count, charset);

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


//----------------------------------------------------------------------------
// Encode this UTF-16 string into a DVB string (preceded by its one-byte length).
//----------------------------------------------------------------------------

ts::ByteBlock ts::UString::toDVBWithByteLength(size_type start, size_type count, const DVBCharset* charset) const
{
    if (start >= length()) {
        // Empty string, return one byte containing 0 (the length).
        return ByteBlock(1, 0);
    }
    else {
        // The maximum number of DVB bytes is 255 so that the size fits in one byte.
        ByteBlock bb(256);

        // Convert the string.
        uint8_t* buffer = bb.data() + 1;
        size_type size = bb.size() - 1;
        toDVB(buffer, size, start, count, charset);

        // Truncate unused bytes.
        assert(size < bb.size());
        bb.resize(bb.size() - size);

        // Update length byte.
        bb[0] = uint8_t(bb.size() - 1);
        return bb;
    }
}


//----------------------------------------------------------------------------
// Format a string using a template and arguments.
//----------------------------------------------------------------------------

void ts::UString::format(const UChar* fmt, const std::initializer_list<ArgMixIn>& args)
{
    // Pre-reserve some space. We don't really know how much. Just address the most comman cases.
    reserve(256);

    // Process the string.
    ArgMixInContext ctx(*this, fmt, args);
}

ts::UString ts::UString::Format(const UChar* fmt, const std::initializer_list<ts::ArgMixIn>& args)
{
    UString result;
    result.format(fmt, args);
    return result;
}


//----------------------------------------------------------------------------
// Scan this string for integer or character values.
//----------------------------------------------------------------------------

bool ts::UString::scan(size_t& extractedCount, size_type& endIndex, const UChar* fmt, const std::initializer_list<ArgMixOut>& args) const
{
    // Process this string instance.
    const UChar* input = data();
    ArgMixOutContext ctx(extractedCount, input, fmt, args);

    // Compute the next index in the input string.
    endIndex = input - data();

    // Return true when both the input string and the format have been completely consumed.
    return *input == CHAR_NULL && *fmt == CHAR_NULL;
}


//----------------------------------------------------------------------------
// Debugging support for Format and Scan.
//----------------------------------------------------------------------------

volatile bool ts::UString::ArgMixContext::_debugOn = false;
volatile bool ts::UString::ArgMixContext::_debugValid = false;

ts::UString::ArgMixContext::ArgMixContext(const UChar* fmt, bool output) :
    _fmt(fmt),
    _format(fmt),
    _output(output)
{
}

bool ts::UString::ArgMixContext::debugInit()
{
    _debugOn = ts::EnvironmentExists(u"TSDUCK_FORMAT_DEBUG");
    _debugValid = true;
    return _debugOn;
}

void ts::UString::ArgMixContext::debug(const UString& message, UChar cmd) const
{
    if (debugActive()) {
        std::cerr << (_output ? "[FORMATDBG] " : "[SCANDBG] ") << message;
        if (cmd != CHAR_NULL) {
            std::cerr << " for sequence %" << cmd;
        }
        std::cerr << " at position " << (_fmt - _format) << " in format string: \"" << _format << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// Analysis context of a Format string.
//----------------------------------------------------------------------------

ts::UString::ArgMixInContext::ArgMixInContext(UString& result, const UChar* fmt, const std::initializer_list<ArgMixIn>& args) :
    ArgMixContext(fmt, true),
    _result(result),
    _arg(args.begin()),
    _end(args.end())
{
    // Loop into format, stop at each '%' sequence.
    while (*_fmt != CHAR_NULL) {

        // Locate the next '%' or end of string.
        const UChar* const start = _fmt;
        while (*_fmt != CHAR_NULL && *_fmt != u'%') {
            ++_fmt;
        }

        // Copy this literal sequence directly into the result.
        result.append(start, _fmt - start);

        // Process '%' sequence.
        if (*_fmt == u'%') {
            ++_fmt;
            processArg();
        }
    }

    // Report extraneous parameters.
    if (_arg != _end && debugActive()) {
        debug(u"extraneous " + Decimal(_end - _arg) + u" arguments");
    }
}

// Anciliary function to process one '%' sequence.
void ts::UString::ArgMixInContext::processArg()
{
    // Invalid '%' at end of string.
    if (*_fmt == CHAR_NULL) {
        return;
    }

    // Process literal '%'.
    if (*_fmt == u'%') {
        _result.push_back(u'%');
        ++_fmt;
        return;
    }

    // The allowed options, between the '%' and the letter are:
    //       - : Left-justified (right-justified by default).
    //       + : Force a '+' sign with decimal integers.
    //       0 : Zero padding for integers.
    //  digits : Minimum field width.
    // .digits : Maximum field width or precision for floating point values.
    //       ' : For integer conversions, use a separator for groups of thousands.
    //       * : Can be used instead of @e digits. The integer value is taken from the argument list.

    bool leftJustified = false;
    bool forceSign = false;
    bool useSeparator = false;
    UChar pad = u' ';
    size_t minWidth = 0;
    size_t maxWidth = std::numeric_limits<size_t>::max();
    size_t precision = 6;

    if (*_fmt == u'-') {
        leftJustified = true;
        _fmt++;
    }
    if (*_fmt == u'+') {
        forceSign = true;
        _fmt++;
    }
    if (*_fmt == u'0') {
        pad = u'0';
        _fmt++;
    }
    getFormatSize(minWidth);
    if (*_fmt == u'.') {
        ++_fmt;
        getFormatSize(maxWidth);
        precision = maxWidth;
        if (maxWidth < minWidth) {
            maxWidth = minWidth;
        }
    }
    if (*_fmt == u'\'') {
        useSeparator = true;
        _fmt++;
    }

    // The thousands separator to use.
    const UString& separator(useSeparator ? DEFAULT_THOUSANDS_SEPARATOR : EMPTY);

    // The available '%' sequences are:
    // - %s : String.
    // - %c : Character.
    // - %d : Integer in decimal.
    // - %x : Integer in lowercase hexadecimal.
    // - %X : Integer in uppercase hexadecimal.
    // - %f : Floating point value.
    // - %% : Insert a literal % (already done).

    // Extract the command and set fmt to its final value, after the '%' sequence.
    const UChar cmd = *_fmt;
    if (cmd != CHAR_NULL) {
        ++_fmt;
    }

    // Process invalid '%' sequence.
    if (cmd != u's' && cmd != u'c' && cmd != u'd' && cmd != u'x' && cmd != u'X' && cmd != u'f') {
        if (debugActive()) {
            debug(u"invalid '%' sequence", cmd);
        }
        return;
    }

    // Process missing argument.
    if (_arg == _end) {
        if (debugActive()) {
            debug(u"missing argument", cmd);
        }
        return;
    }

    // Now, the command is valid, process it.
    if (_arg->isAnyString() || (_arg->isBool() && cmd == u's')) {
        // String arguments are always treated as %s, regardless of the % command.
        // Also if a bool is specified as %s, print true or false.
        if (cmd != u's' && debugActive()) {
            debug(u"type mismatch, got a string", cmd);
        }
        // Get the string parameter.
        UString value;
        if (_arg->isAnyString8()) {
            value.assignFromUTF8(_arg->toCharPtr());
        }
        else if (_arg->isAnyString16()) {
            value.assign(_arg->toUCharPtr());
        }
        else if (_arg->isBool()) {
            value.assign(TrueFalse(_arg->toBool()));
        }
        else {
            // Not a string, should not get there.
            assert(false);
        }
        // Truncate the string.
        size_t wid = value.width();
        if (maxWidth < wid) {
            value.truncateWidth(maxWidth, leftJustified ? LEFT_TO_RIGHT : RIGHT_TO_LEFT);
            wid = maxWidth;
        }
        // Insert the string with optional padding.
        if (minWidth > wid && !leftJustified) {
            _result.append(minWidth - wid, pad);
        }
        _result.append(value);
        if (minWidth > wid && leftJustified) {
            _result.append(minWidth - wid, pad);
        }
    }
    else if (cmd == u'c') {
        // Use an integer value as an Unicode code point.
        if (!_arg->isInteger() && debugActive()) {
            debug(u"type mismatch, not an integer or character", cmd);
        }
        // Get and convert the Unicode code point.
        _result.append(_arg->toUInt32());
    }
    else if (cmd == u'x' || cmd == u'X') {
        // Insert an integer in hexadecimal.
        if (!_arg->isInteger() && debugActive()) {
            debug(u"type mismatch, not an integer", cmd);
        }
        // Format the hexa string.
        const bool upper = cmd == u'X';
        switch (_arg->size()) {
            case 1:
                _result.append(HexaMin(_arg->toInteger<uint8_t>(), minWidth, separator, false, upper));
                break;
            case 2:
                _result.append(HexaMin(_arg->toInteger<uint16_t>(), minWidth, separator, false, upper));
                break;
            case 4:
                _result.append(HexaMin(_arg->toInteger<uint32_t>(), minWidth, separator, false, upper));
                break;
            default:
                _result.append(HexaMin(_arg->toInteger<uint64_t>(), minWidth, separator, false, upper));
                break;
        }
    }
    else if (cmd == u'f') {
        // Insert a floating point value
        if (!_arg->isDouble() && debugActive()) {
            debug(u"type mismatch, not a double", cmd);
        }
        _result.append(Float(_arg->toDouble(), minWidth, precision, forceSign));
    }
    else {
        // Insert an integer in decimal.
        if (cmd != u'd' && debugActive()) {
            debug(u"type mismatch, got an integer", cmd);
        }
        if (_arg->size() > 4) {
            // Stored as 64-bit integer.
            if (_arg->isSigned()) {
                _result.append(Decimal(_arg->toInt64(), minWidth, !leftJustified, separator, forceSign, pad));
            }
            else {
                _result.append(Decimal(_arg->toUInt64(), minWidth, !leftJustified, separator, forceSign, pad));
            }
        }
        else {
            // Stored as 32-bit integer.
            if (_arg->isSigned()) {
                _result.append(Decimal(_arg->toInt32(), minWidth, !leftJustified, separator, forceSign, pad));
            }
            else {
                _result.append(Decimal(_arg->toUInt32(), minWidth, !leftJustified, separator, forceSign, pad));
            }
        }
    }

    // Finally, absorb the inserted argument.
    ++_arg;
}

// Anciliary function to extract a size field from a '%' sequence.
void ts::UString::ArgMixInContext::getFormatSize(size_t& size)
{
    if (IsDigit(*_fmt)) {
        // An decimal integer literal is present, decode it.
        size = 0;
        while (IsDigit(*_fmt)) {
            size = 10 * size + *_fmt++ - u'0';
        }
    }
    else if (*_fmt == u'*') {
        // The size field is taken from the argument list.
        ++_fmt;
        if (_arg != _end) {
            size = _arg->toInteger<size_t>();
            ++_arg;
        }
        else if (debugActive()) {
            debug(u"missing argument for %* specifier");
        }
    }
}


//----------------------------------------------------------------------------
// Analysis context of a scan string.
//----------------------------------------------------------------------------

ts::UString::ArgMixOutContext::ArgMixOutContext(size_t& extractedCount, const UChar*& input, const UChar*& fmt, const std::initializer_list<ArgMixOut>& args) :
    ArgMixContext(fmt, false),
    _input(input),
    _arg(args.begin()),
    _end(args.end())
{
    // Initialize output fields.
    extractedCount = 0;

    // Process all fields until end of any string or mismatch.
    do {
        // Skip spaces in input and format to point to next meaningful field.
        skipSpaces(_input);
        skipSpaces(_fmt);
    } while (*_input != CHAR_NULL && *_fmt != CHAR_NULL && processField());

    // Return updated pointers.
    extractedCount = _arg - args.begin();
    input = _input;
    fmt = _fmt;

    // Report extraneous parameters if the format has been completely parsed.
    if (*_fmt == CHAR_NULL && _arg != _end && debugActive()) {
        debug(u"extraneous " + Decimal(_end - _arg) + u" arguments");
    }
}

// Skip space sequences in a string.
void ts::UString::ArgMixOutContext::skipSpaces(const UChar*& s)
{
    while (IsSpace(*s)) {
        ++s;
    }
}

// Process one field, either a literal character or a '%' sequence.
// Return true on match, false on error.
bool ts::UString::ArgMixOutContext::processField()
{
    assert(*_fmt != CHAR_NULL);
    assert(*_input != CHAR_NULL);

    // Process literal characters.
    if (*_fmt != u'%' || _fmt[1] == u'%') {
        // Either not a '%' sequence or a '%%' meaning a literal '%'.
        if (*_input != *_fmt) {
            // Failed to match a literal character.
            return false;
        }
        else {
            // The literal character matched, advance pointers.
            ++_input;
            _fmt += *_fmt == '%' ? 2 : 1;
            return true;
        }
    }

    // The available '%' sequences are:
    // - %d : Matches an integer in decimal or hexadecimal.
    // - %x : Matches an integer in hexadecimal, case-insensitive, without 0x or 0X prefix.
    // - %X : Same as %x.
    // - %c : Matches the next non-space character. The Unicode code point is returned.
    // - %% : Matches a literal % (already done).
    // The allowed options, between the '%' and the letter are:
    //    ' : For decimal integer conversions, skip separators for groups of thousands.

    // Extract the command and set fmt to its final value, after the '%' sequence.
    bool skipSeparator = false;
    UChar cmd = *++_fmt;
    if (cmd == u'\'') {
        skipSeparator = true;
        cmd = *++_fmt;
    }
    if (cmd != CHAR_NULL) {
        ++_fmt;
    }

    // Process invalid '%' sequence.
    if (cmd != u'c' && cmd != u'd' && cmd != u'i' && cmd != u'x' && cmd != u'X') {
        if (debugActive()) {
            debug(u"invalid '%' sequence", cmd);
        }
        return false;
    }

    // Process missing argument.
    if (_arg == _end) {
        if (debugActive()) {
            debug(u"missing argument", cmd);
        }
        return false;
    }

    // Process incorrect argument (internal error, bug).
    if (!_arg->isOutputInteger()) {
        // This should never occur since ArgMixOut can be constructed only from pointer to integer.
        debug(u"internal error, scan() argument is not a pointer to integer");
        return false;
    }

    // Extract a character literal, return its Unicode code point.
    if (cmd == u'c') {
        (_arg++)->storeInteger(*_input++);
        return true;
    }

    // Extract an integer value.
    UString value;
    const UChar* const start = _input;

    if (cmd == u'x' || cmd == u'X') {
        // Extract an hexadecimal value, without prefix.
        while (IsHexa(*_input)) {
            _input++;
        }
        // Extract the hexadecimal value with an added prefix.
        value = u"0x";
    }
    else if (_input[0] == u'0' && (_input[1] == u'x' || _input[1] == u'X')) {
        // Extract an hexadecimal value with prefix.
        if (IsHexa(_input[2])) {
            _input += 3;
            while (IsHexa(*_input)) {
                _input++;
            }
        }
    }
    else {
        // Extract a decimal value.
        if (_input[0] == u'-' && IsDigit(_input[1])) {
            _input += 2;
        }
        while (IsDigit(*_input) || (skipSeparator && *_input == u',')) {
            _input++;
        }
    }

    // Process value not found, invalid input, not a programming error.
    if (_input == start) {
        // No hexa value found
        return false;
    }

    // Build the string to decode, preserve optional prefix we added.
    value.append(start, _input - start);
    if (skipSeparator) {
        value.remove(u',');
    }

    // Decode signed or usigned value. Use 64 bits in all cases.
    // Note the decoding should not fail since we already checked the syntax.
    if (_arg->isSigned()) {
        int64_t i = 0;
        value.toInteger(i);
        _arg->storeInteger(i);
    }
    else {
        uint64_t i = 0;
        value.toInteger(i);
        _arg->storeInteger(i);
    }

    // Finally, absorb the extracted argument.
    ++_arg;
    return true;
}


//----------------------------------------------------------------------------
// Format a string containing a floating point value.
//----------------------------------------------------------------------------

ts::UString ts::UString::Float(double value, size_type width, size_type precision, bool force_sign)
{
    // Slightly oversized buffer.
    char valueStr[10 + std::numeric_limits<double>::digits - std::numeric_limits<double>::min_exponent];
    if (force_sign) {
        std::snprintf(valueStr, sizeof(valueStr), "%+*.*f", int(width), int(precision), value);
    }
    else {
        std::snprintf(valueStr, sizeof(valueStr), "%*.*f", int(width), int(precision), value);
    }
    return FromUTF8(valueStr);
}
