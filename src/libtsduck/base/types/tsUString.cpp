//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

// In the implementation of UString, we allow the implicit UTF-8 conversions
// to ensure that the symbols are defined and exported from the DLL on Windows.
// Otherwise, applications may get undefined symbols if they allow these
// implicit conversions.
#define TS_ALLOW_IMPLICIT_UTF8_CONVERSION 1

#include "tsUString.h"
#include "tsByteBlock.h"
#include "tsEnvironment.h"
#include "tsIntegerUtils.h"
#include "tsEnumeration.h"

// The UTF-8 Byte Order Mark
const char* const ts::UString::UTF8_BOM = "\xEF\xBB\xBF";

// End-of-line sequence for the operating system..
const ts::UString ts::UString::EOL
#if defined(TS_WINDOWS)
    ("\r\n");
#else
    ("\n");
#endif

// Default separator string for groups of thousands, a comma.
const ts::UString ts::UString::DEFAULT_THOUSANDS_SEPARATOR(1, u',');

// A reference empty string.
const ts::UString ts::UString::EMPTY;

// The default list of characters to be protected by quoted().
const ts::UString ts::UString::DEFAULT_SPECIAL_CHARACTERS(u"\"'`;$*?&(){}[]");
const ts::UString ts::UString::DEFAULT_QUOTE_CHARACTERS(u"\"'");


//----------------------------------------------------------------------------
// Conversions with Windows Unicode strings (Windows-specific).
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS) || defined(DOXYGEN)

// Constructor using a Windows Unicode string.
ts::UString::UString(const ::WCHAR* s, size_type count, const allocator_type& alloc) :
    UString(reinterpret_cast<const UChar*>(s), count, alloc)
{
    assert(sizeof(::WCHAR) == sizeof(UChar));
}

// Constructor using a Windows Unicode string.
ts::UString::UString(const ::WCHAR* s, const allocator_type& alloc) :
    UString(s == 0 ? &CHAR_NULL : reinterpret_cast<const UChar*>(s), alloc)
{
    assert(sizeof(::WCHAR) == sizeof(UChar));
}

// Get the address of the underlying null-terminated Unicode string.
const ::WCHAR* ts::UString::wc_str() const
{
    assert(sizeof(::WCHAR) == sizeof(UChar));
    return reinterpret_cast<const ::WCHAR*>(data());
}

// Get the address of the underlying null-terminated Unicode string.
::WCHAR* ts::UString::wc_str()
{
    assert(sizeof(::WCHAR) == sizeof(UChar));
    return reinterpret_cast<::WCHAR*>(const_cast<UChar*>(data()));
}

#endif


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
                *outStart++ = uint16_t((code & 0x0F) << 12) | uint16_t((uint16_t(inStart[0] & 0x3F)) << 6) | (inStart[1] & 0x3F);
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
    // The maximum number of UTF-8 bytes is 3 times the number of UTF-16 codes.
    utf8.resize(3 * size());

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
// Convert a C++ "wide string" into a new UString.
//----------------------------------------------------------------------------

ts::UString ts::UString::FromWChar(const std::wstring& wstr)
{
    UString str;
    str.assignFromWChar(wstr);
    return str;
}

ts::UString ts::UString::FromWChar(const wchar_t* wstr)
{
    UString str;
    str.assignFromWChar(wstr);
    return str;
}

ts::UString ts::UString::FromWChar(const wchar_t* wstr, size_type count)
{
    UString str;
    str.assignFromWChar(wstr, count);
    return str;
}


//----------------------------------------------------------------------------
// Convert a C++ "wide string" into this object.
//----------------------------------------------------------------------------

ts::UString& ts::UString::assignFromWChar(const wchar_t* wstr)
{
    return assignFromWChar(wstr, wstr == nullptr ? 0 : ::wcslen(wstr));
}

ts::UString& ts::UString::assignFromWChar(const wchar_t* wstr, size_type count)
{
    if (wstr == nullptr) {
        clear();
    }
    else {
#if TS_WCHAR_SIZE == 1
        // Convert from UTF-8.
        assignFromUTF8(reinterpret_cast<const char*>(wstr), count);
#elif TS_WCHAR_SIZE == 2
        // Already in UTF-16, direct binary copy.
        resize(count);
        std::memcpy(&(*this)[0], wstr, 2 * count);
#else
        // Assume that wchar_t is a full Unicode code point.
        while (count-- > 0) {
            const char32_t cp = *wstr++;
            if (NeedSurrogate(cp)) {
                append(LeadingSurrogate(cp));
                append(TrailingSurrogate(cp));
            }
            else {
                append(UChar(cp));
            }
        }
#endif
    }
    return *this;
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

void ts::UString::trim(bool leading, bool trailing, bool sequences)
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
    if (sequences) {
        bool in_space = false;
        size_type increment = 0;
        for (size_type index = 0; index < length(); index += increment) {
            if (!IsSpace((*this)[index])) {
                // Out of space sequence
                in_space = false;
                increment = 1;
            }
            else if (in_space) {
                // Middle of space sequence, erase.
                erase(index, 1);
                increment = 0;
            }
            else {
                // Start of space sequence, replace with a plain space.
                (*this)[index] = SPACE;
                in_space = true;
                increment = 1;
            }
        }
    }
}

ts::UString ts::UString::toTrimmed(bool leading, bool trailing, bool sequences) const
{
    UString result(*this);
    result.trim(leading, trailing, sequences);
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
// Convert between precombined characters and sequences of two characters.
//----------------------------------------------------------------------------

void ts::UString::combineDiacritical()
{
    size_type cur = 0;  // overwrite pointer
    UChar precomb = 0;  // precombined replacement character

    for (size_type old = 0; old < length(); ++old) {
        if (old > 0 && IsCombiningDiacritical(at(old)) && (precomb = Precombined(at(old-1), at(old))) != CHAR_NULL) {
            // This is a replaceable combination.
            assert(cur > 0);
            at(cur-1) = precomb;
        }
        else {
            // This is a standard character.
            at(cur++) = at(old);
        }
    }

    // Truncate unused characters.
    resize(cur);
}

ts::UString ts::UString::toCombinedDiacritical() const
{
    UString result(*this);
    result.combineDiacritical();
    return result;
}

void ts::UString::decomposeDiacritical()
{
    const size_type len = length();
    UString rep;  // replacement for new string.
    UChar letter = 0;
    UChar mark = 0;

    // Reserve memory for the result (at most 2 out characters for one in character).
    rep.reserve(2 * len);

    for (size_type i = 0; i < length(); ++i) {
        if (DecomposePrecombined(at(i), letter, mark)) {
            // This is a precombined character and we decomposed it.
            rep.append(letter);
            rep.append(mark);
        }
        else {
            // Not a precombined character.
            rep.append(at(i));
        }
    }

    // If many case, the replacement is identical to the old string.
    // When they are different, their sizes are different as well.
    if (rep.length() != length()) {
        swap(rep);
    }
}

ts::UString ts::UString::toDecomposedDiacritical() const
{
    UString result(*this);
    result.decomposeDiacritical();
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

void ts::UString::substitute(UChar value, UChar replacement)
{
    if (value != replacement) {
        for (size_t i = 0; i < length(); ++i) {
            if ((*this)[i] == value) {
                (*this)[i] = replacement;
            }
        }
    }
}

ts::UString ts::UString::toSubstituted(const UString& value, const UString& replacement) const
{
    UString result(*this);
    result.substitute(value, replacement);
    return result;
}

ts::UString ts::UString::toSubstituted(UChar value, UChar replacement) const
{
    UString result(*this);
    result.substitute(value, replacement);
    return result;
}


//----------------------------------------------------------------------------
// Indent all lines in the string.
//----------------------------------------------------------------------------

void ts::UString::indent(size_t count)
{
    if (count > 0) {
        bool atbol = true; // at beginning of a line
        for (size_type i = 0; i < size(); ++i) {
            const UChar c = at(i);
            if (c == LINE_FEED) {
                atbol = true;
            }
            else if (atbol && !IsSpace(c)) {
                atbol = false;
                insert(i, count, SPACE);
                i += count;
            }
        }
    }
}

ts::UString ts::UString::toIndented(size_t count) const
{
    UString result(*this);
    result.indent(count);
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

bool ts::UString::startWith(const UString& prefix, CaseSensitivity cs, bool skip_space) const
{
    const size_type len = length();
    const size_type sublen = prefix.length();

    size_type start = 0;
    if (skip_space) {
        while (start < len && IsSpace(at(start))) {
            ++start;
        }
    }

    if (len < start + sublen) {
        return false;
    }

    switch (cs) {
        case CASE_SENSITIVE: {
            return compare(start, sublen, prefix) == 0;
        }
        case CASE_INSENSITIVE: {
            for (size_type i = 0; i < sublen; ++i) {
                if (ToLower(at(start + i)) != ToLower(prefix.at(i))) {
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

bool ts::UString::endWith(const UString& suffix, CaseSensitivity cs, bool skip_space) const
{
    size_type iString = length();
    size_type iSuffix = suffix.length();

    if (skip_space) {
        while (iString > 0 && IsSpace(at(iString - 1))) {
            --iString;
        }
    }

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

bool ts::UString::contain(UChar c) const
{
    return find(c) != NPOS;
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
// Compute the number of similar leading/trailing characters in two strings.
//----------------------------------------------------------------------------

size_t ts::UString::commonPrefixSize(const ts::UString &str, ts::CaseSensitivity cs) const
{
    const size_t len = std::min(length(), str.length());
    for (size_t i = 0; i < len; ++i) {
        if (cs == CASE_SENSITIVE) {
            if (at(i) != str.at(i)) {
                return i;
            }
        }
        else {
            if (ToLower(at(i)) != ToLower(str.at(i))) {
                return i;
            }
        }
    }
    return len;
}

size_t ts::UString::commonSuffixSize(const ts::UString &str, ts::CaseSensitivity cs) const
{
    const size_t len1 = length();
    const size_t len2 = str.length();
    const size_t len = std::min(len1, len2);
    for (size_t i = 0; i < len; ++i) {
        if (cs == CASE_SENSITIVE) {
            if (at(len1 - i - 1) != str.at(len2 - i - 1)) {
                return i;
            }
        }
        else {
            if (ToLower(at(len1 - i - 1)) != ToLower(str.at(len2 - i - 1))) {
                return i;
            }
        }
    }
    return len;
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
// Replace the string with a "quoted" version of it.
//----------------------------------------------------------------------------

ts::UString ts::UString::toQuoted(UChar quoteCharacter, const UString& specialCharacters, bool forceQuote) const
{
    UString result(*this);
    result.quoted(quoteCharacter, specialCharacters, forceQuote);
    return result;
}

void ts::UString::quoted(UChar quoteCharacter, const UString& specialCharacters, bool forceQuote)
{
    // Check if the string contains any character which requires quoting.
    // An empty string needs to be quoted as well to be identified as an actual empty string.
    bool needQuote = forceQuote || empty();
    for (size_type i = 0; !needQuote && i < size(); ++i) {
        const UChar c = at(i);
        needQuote = c == '\\' || c == quoteCharacter || IsSpace(c) || specialCharacters.contain(c);
    }

    // Perform quoting only if needed.
    if (needQuote) {
        // Opening quote.
        insert(0, 1, quoteCharacter);
        // Loop on all characters. Skip new opening quote.
        for (size_type i = 1; i < size(); ++i) {
            const UChar c = at(i);
            if (c == '\\' || c == quoteCharacter) {
                // This character must be escaped.
                insert(i++, 1, '\\');
            }
            else if (IsSpace(c)) {
                // A space character is either a plain space or a specific escape sequence.
                UChar rep = CHAR_NULL;
                switch (c) {
                    case BACKSPACE: rep = u'b'; break;
                    case FORM_FEED: rep = u'f'; break;
                    case LINE_FEED: rep = u'n'; break;
                    case CARRIAGE_RETURN: rep = u'r'; break;
                    case HORIZONTAL_TABULATION: rep = u't'; break;
                    default: break;
                }
                if (rep == CHAR_NULL) {
                    // No escape sequence defined, make sure it is just a space.
                    at(i) = SPACE;
                }
                else {
                    // An escape sequence is defined.
                    insert(i++, 1, '\\');
                    at(i) = rep;
                }
            }
        }
        // Final quote.
        push_back(quoteCharacter);
    }
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
    const int64_t k = 1024;

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
// Compare two strings using various comparison options.
//----------------------------------------------------------------------------

int ts::UString::SuperCompare(const UChar* s1, const UChar* s2, uint32_t flags)
{
    // Eliminate trivial cases with null pointers.
    if (s1 == nullptr || s2 == nullptr) {
        return (s1 == nullptr && s2 == nullptr) ? 0 : (s1 == nullptr ? -1 : 1);
    }

    // Loop on characters in both strings.
    for (;;) {
        // Characteristics of current character in each strings.
        uint32_t ccc1 = UCharacteristics(*s1);
        uint32_t ccc2 = UCharacteristics(*s2);

        // Skip spaces if required (null char is not a space).
        if (flags & SCOMP_IGNORE_BLANKS) {
            while (ccc1 & CCHAR_SPACE) {
                ccc1 = UCharacteristics(*++s1);
            }
            while (ccc2 & CCHAR_SPACE) {
                ccc2 = UCharacteristics(*++s2);
            }
        }

        // Manage end of string.
        if (*s1 == CHAR_NULL) {
            return *s2 == CHAR_NULL ? 0 : -1;
        }
        if (*s2 == CHAR_NULL) {
            return 1;
        }

        if ((flags & SCOMP_CASE_INSENSITIVE) && (ccc1 & CCHAR_LETTER) && (ccc2 & CCHAR_LETTER)) {
            // Manage case insensitive comparison.
            const UChar c1 = ToLower(*s1++);
            const UChar c2 = ToLower(*s2++);
            if (c1 != c2) {
                return c1 < c2 ? -1 : 1;
            }
        }
        else if ((flags & SCOMP_NUMERIC) && (ccc1 & CCHAR_DIGIT) && (ccc2 & CCHAR_DIGIT)) {
            // Manage numeric fields.
            uint64_t i1 = 0;
            uint64_t i2 = 0;
            while (ccc1 & CCHAR_DIGIT) {
                i1 = (10 * i1) + (*s1 - DIGIT_ZERO);
                ccc1 = UCharacteristics(*++s1);
            }
            while (ccc2 & CCHAR_DIGIT) {
                i2 = (10 * i2) + (*s2 - DIGIT_ZERO);
                ccc2 = UCharacteristics(*++s2);
            }
            if (i1 != i2) {
                return i1 < i2 ? -1 : 1;
            }
        }
        else {
            // Character comparison, including surrogate pairs.
            char32_t c1 = *s1++;
            char32_t c2 = *s2++;
            if (IsLeadingSurrogate(UChar(c1)) && IsTrailingSurrogate(*s1)) {
                c1 = FromSurrogatePair(UChar(c1), *s1++);
            }
            if (IsLeadingSurrogate(UChar(c2)) && IsTrailingSurrogate(*s2)) {
                c2 = FromSurrogatePair(UChar(c2), *s2++);
            }
            if (c1 != c2) {
                return c1 < c2 ? -1 : 1;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Check if two strings are identical, case-insensitive and ignoring blanks
//----------------------------------------------------------------------------

bool ts::UString::similar(const void* addr, size_type size) const
{
    return addr != nullptr && similar(FromUTF8(reinterpret_cast<const char*>(addr), size));
}


//----------------------------------------------------------------------------
// Save this string into a file, in UTF-8 format.
//----------------------------------------------------------------------------

bool ts::UString::save(const fs::path& fileName, bool append, bool enforceLastLineFeed) const
{
    std::ofstream file(fileName, append ? (std::ios::out | std::ios::app) : std::ios::out);
    file << *this;
    if (enforceLastLineFeed && !empty() && back() != LINE_FEED) {
        // Check if the first end of line is a LF or CR/LF.
        // Use the same eol sequence for the last one, regardless of the system.
        const size_type lf = find(LINE_FEED);
        if (lf != NPOS && lf > 0 && (*this)[lf-1] == CARRIAGE_RETURN) {
            // The first eol is a CR/LF.
            file << "\r\n";
        }
        else {
            file << '\n';
        }
    }
    file.close();
    return !file.fail();
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
        value = Tristate::Maybe;
        return false;
    }
    else {
        // Valid string or integer.
        switch (iValue) {
            case TSE_FALSE:
            case TSE_NO:
            case TSE_OFF:
                value = Tristate::False;
                break;
            case TSE_TRUE:
            case TSE_YES:
            case TSE_ON:
                value = Tristate::True;
                break;
            case TSE_MAYBE:
            case TSE_UNKNOWN:
                value = Tristate::Maybe;
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

bool ts::UString::hexaDecode(ByteBlock& result, bool c_style) const
{
    result.clear();
    return hexaDecodeAppend(result, c_style);
}

bool ts::UString::hexaDecodeAppend(ts::ByteBlock& result, bool c_style) const
{
    // Oversize the prereservation in output buffer.
    result.reserve(result.size() + size() / 2);

    bool got_first_nibble = false;
    uint8_t byte = 0;
    uint8_t nibble = 0;

    for (const UChar* p = data(); p < last(); ++p) {
        if (IsSpace(*p) || (c_style && (*p == ',' || *p == ';' || *p == '[' || *p == ']' || *p == '{' || *p == '}'))) {
            // Ignore spaces and C-style separators.
            continue;
        }
        else if (c_style && *p == '0' && p + 1 < last() && (p[1] == 'x' || p[1] == 'X')) {
            // Ignore C-style 0x prefix.
            ++p;
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
            byte = uint8_t(nibble << 4);
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
    // Do nothing in case of invalid or empty data.
    if (data == nullptr || size == 0) {
        return;
    }

    // Work an area of bytes.
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
// Format a string using a template and arguments.
//----------------------------------------------------------------------------

void ts::UString::format(const UChar* fmt, std::initializer_list<ArgMixIn> args)
{
    // Pre-reserve some space. We don't really know how much. Just address the most common cases.
    reserve(256);

    // Process the string.
    ArgMixInContext ctx(*this, fmt, args);
}

ts::UString ts::UString::Format(const UChar* fmt, std::initializer_list<ArgMixIn> args)
{
    UString result;
    result.format(fmt, args);
    return result;
}


//----------------------------------------------------------------------------
// Scan this string for integer or character values.
//----------------------------------------------------------------------------

bool ts::UString::scan(size_t& extractedCount, size_type& endIndex, const UChar* fmt, std::initializer_list<ArgMixOut> args) const
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
    _debugOn = EnvironmentExists(u"TSDUCK_FORMAT_DEBUG");
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

ts::UString::ArgMixInContext::ArgMixInContext(UString& result, const UChar* fmt, std::initializer_list<ArgMixIn> args) :
    ArgMixContext(fmt, true),
    _result(result),
    _arg(args.begin()),
    _prev(args.end()),
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
    //       < : Reuse previous argument value, do not advance in argument list.
    //       - : Left-justified (right-justified by default).
    //       + : Force a '+' sign with decimal integers.
    //       0 : Zero padding for integers.
    //  digits : Minimum field width.
    // .digits : Maximum field width or precision for floating/fixed point values.
    //       ' : For integer conversions, use a separator for groups of thousands.
    //       * : Can be used instead of @e digits. The integer value is taken from the argument list.

    bool leftJustified = false;
    bool forceSign = false;
    bool useSeparator = false;
    bool reusePrevious = false;
    bool hasDot = false;
    UChar pad = u' ';
    size_t minWidth = 0;
    size_t maxWidth = std::numeric_limits<size_t>::max();
    size_t precision = 0;

    if (*_fmt == u'<') {
        reusePrevious = true;
        _fmt++;
    }
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
        hasDot = true;
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
    const UChar separatorChar = useSeparator ? COMMA : CHAR_NULL;

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

    // Point to actual parameter value.
    ArgIterator argit(_arg);
    if (reusePrevious) {
        // Reuse previous argument value, do not advance in argument list.
        argit = _prev;
    }
    else {
        // Absorb the inserted argument.
        _prev = _arg++;
    }

    // Process missing argument.
    if (argit == _end) {
        if (debugActive()) {
            debug(u"missing argument", cmd);
        }
        return;
    }

    // Now, the command is valid, process it.
    if (argit->isAnyString() || (argit->isBool() && cmd == u's')) {
        // String arguments are always treated as %s, regardless of the % command.
        // Also if a bool is specified as %s, print true or false.
        if (cmd != u's' && debugActive()) {
            debug(u"type mismatch, got a string", cmd);
        }
        // Get the string parameter.
        UString value;
        if (argit->isAnyString8()) {
            value.assignFromUTF8(argit->toCharPtr());
        }
        else if (argit->isAnyString16()) {
            value.assign(argit->toUCharPtr());
        }
        else if (argit->isBool()) {
            value.assign(TrueFalse(argit->toBool()));
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
    else if (argit->isAbstractNumber() && cmd == u's') {
        // An AbstractNumber using the most general string format.
        _result.append(argit->toAbstractNumber().toString(minWidth, !leftJustified, separatorChar, forceSign, precision > 0 ? precision : NPOS, false, FULL_STOP, pad));
    }
    else if (cmd == u'c') {
        // Use an integer value as an Unicode code point.
        if (!argit->isInteger() && debugActive()) {
            debug(u"type mismatch, not an integer or character", cmd);
        }
        // Get and convert the Unicode code point.
        _result.append(argit->toUInt32());
    }
    else if (cmd == u'x' || cmd == u'X') {
        // Insert an integer in hexadecimal.
        if (!argit->isInteger() && !argit->isAbstractNumber() && debugActive()) {
            debug(u"type mismatch, not an integer", cmd);
        }
        // Format the hexa string.
        const bool upper = cmd == u'X';
        if (argit->isAbstractNumber()) {
            _result.append(HexaMin(argit->toInteger<uint64_t>(), minWidth, separator, false, upper));
        }
        else {
            switch (argit->size()) {
                case 1:
                    _result.append(HexaMin(argit->toInteger<uint8_t>(), minWidth, separator, false, upper));
                    break;
                case 2:
                    _result.append(HexaMin(argit->toInteger<uint16_t>(), minWidth, separator, false, upper));
                    break;
                case 4:
                    _result.append(HexaMin(argit->toInteger<uint32_t>(), minWidth, separator, false, upper));
                    break;
                default:
                    _result.append(HexaMin(argit->toInteger<uint64_t>(), minWidth, separator, false, upper));
                    break;
            }
        }
    }
    else if (cmd == u'f') {
        // Insert a floating point value
        if (!argit->isDouble() && !argit->isAbstractNumber() && debugActive()) {
            debug(u"type mismatch, not a double or fixed-point", cmd);
        }
        if (argit->isAbstractNumber()) {
            _result.append(argit->toAbstractNumber().toString(minWidth, !leftJustified, separatorChar, forceSign, precision > 0 ? precision : NPOS, hasDot, FULL_STOP, pad));
        }
        else {
            // Get a float or convert an integer to a float. Default to 6 decimal digits.
            _result.append(Float(argit->toDouble(), minWidth, precision > 0 ? precision : 6, forceSign));
        }
    }
    else {
        // Insert an integer in decimal.
        if (cmd != u'd' && debugActive()) {
            debug(u"type mismatch, got an integer", cmd);
        }
        if (argit->isAbstractNumber()) {
            // Format AbstractNumber without decimals.
            _result.append(argit->toAbstractNumber().toString(minWidth, !leftJustified, separatorChar, forceSign, 0, true, FULL_STOP, pad));
        }
        else if (argit->size() > 4) {
            // Stored as 64-bit integer.
            if (argit->isSigned()) {
                _result.append(Decimal(argit->toInt64(), minWidth, !leftJustified, separator, forceSign, pad));
            }
            else {
                _result.append(Decimal(argit->toUInt64(), minWidth, !leftJustified, separator, forceSign, pad));
            }
        }
        else {
            // Stored as 32-bit integer.
            if (argit->isSigned()) {
                _result.append(Decimal(argit->toInt32(), minWidth, !leftJustified, separator, forceSign, pad));
            }
            else {
                _result.append(Decimal(argit->toUInt32(), minWidth, !leftJustified, separator, forceSign, pad));
            }
        }
    }
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

ts::UString::ArgMixOutContext::ArgMixOutContext(size_t& extractedCount, const UChar*& input, const UChar*& fmt, std::initializer_list<ArgMixOut> args) :
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
    // Default precision is 6 decimal digits.
    const bool no_size = width == 0 && precision == 0;
    if (precision == 0) {
        precision = 6;
    }

    // Build formatting string.
    std::string format("%");
    if (force_sign) {
        format.append("+");
    }
    format.append("*.*l");
    const double avalue = std::fabs(value);
    // Use "f" format if value is greater than this, "e" format if lower
    const double min_f_value = precision > 1 && precision <= MAX_POWER_10 ? 1.0 / double(Power10(precision / 2)) : 0.000001;
    if (avalue < std::numeric_limits<double>::epsilon() || (avalue >= min_f_value && avalue < 100000.0)) {
        // Use a float representation.
        format.append("f");
    }
    else {
        // Use a exponent representation.
        format.append("e");
    }

    // Oversized buffer.
    std::string str(32 + width + std::numeric_limits<double>::digits - std::numeric_limits<double>::min_exponent, '\0');

    // Format the result.
    TS_PUSH_WARNING()
    TS_GCC_NOWARNING(format-nonliteral)
    TS_LLVM_NOWARNING(format-nonliteral)
    TS_MSC_NOWARNING(4774) // 'snprintf' : format string expected in argument 3 is not a string literal
    std::snprintf(&str[0], str.size(), format.c_str(), int(width), int(precision), value);
    TS_POP_WARNING()

    str[str.size() - 1] = '\0';
    UString result;
    result.assignFromUTF8(str.c_str());

    // Cleanup extra zeroes when no formatting rule is given.
    if (no_size) {
        // Find decimal dot and exponent.
        const size_type dot = result.find(u'.');
        const size_type exp = result.find_first_of(u"eE");
        if (exp == NPOS) {
            // No exponent, remove trailing fractional zeroes.
            if (dot != NPOS) {
                while (!result.empty() && result.back() == u'0') {
                    result.pop_back();
                }
            }
            // Remove empty fractional part.
            if (!result.empty() && result.back() == u'.') {
                result.pop_back();
            }
        }
        else {
            // Remove leading zeroes in exponent.
            size_type pos = exp + 1;
            while (pos < result.size() && !IsDigit(result[pos])) {
                pos++;
            }
            while (pos + 1 < result.size() && result[pos] == u'0') {
                result.erase(pos, 1);
            }
            // Remove trailing zeroes in fractional part, but keep a fractional part.
            if (dot != NPOS && exp > 0) {
                for (size_type i = exp - 1; i > dot + 1 && result[i] == u'0'; i--) {
                    result.erase(i, 1);
                }
            }
        }
    }

    return result;
}
