//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#pragma once


//----------------------------------------------------------------------------
// Assign from std::vector and std::array.
//----------------------------------------------------------------------------

// With Microsoft compiler:
// warning C4127: conditional expression is constant
// for expression: if (sizeof(CHARTYPE) == sizeof(UChar)) {
TS_PUSH_WARNING()
TS_MSC_NOWARNING(4127)

template <typename CHARTYPE, typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
ts::UString& ts::UString::assign(const std::vector<CHARTYPE>& vec, INT count)
{
    // The character type must be 16 bits.
    assert(sizeof(CHARTYPE) == sizeof(UChar));
    if (sizeof(CHARTYPE) == sizeof(UChar)) {

        // Maximum number of characters to check.
        // Take care, carefully crafted expression.
        const size_t last = std::min<std::size_t>(vec.size(), static_cast<size_t>(std::max<INT>(0, count)));

        // Compute actual string length.
        size_type n = 0;
        while (n < last && vec[n] != static_cast<CHARTYPE>(0)) {
            ++n;
        }

        // Assign string.
        assign(reinterpret_cast<const UChar*>(vec.data()), n);
    }
    return *this;
}

template <typename CHARTYPE, std::size_t SIZE, typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
ts::UString& ts::UString::assign(const std::array<CHARTYPE, SIZE>& arr, INT count)
{
    // The character type must be 16 bits.
    assert(sizeof(CHARTYPE) == sizeof(UChar));
    if (sizeof(CHARTYPE) == sizeof(UChar)) {

        // Maximum number of characters to check.
        // Take care, carefully crafted expression.
        const std::size_t last = std::min<std::size_t>(arr.size(), static_cast<std::size_t>(std::max<INT>(0, count)));

        // Compute actual string length.
        size_type n = 0;
        while (n < last && arr[n] != static_cast<CHARTYPE>(0)) {
            ++n;
        }

        // Assign string.
        assign(reinterpret_cast<const UChar*>(arr.data()), n);
    }
    return *this;
}

TS_POP_WARNING()

template <typename CHARTYPE>
ts::UString& ts::UString::assign(const std::vector<CHARTYPE>& vec)
{
    return assign(vec, vec.size());
}

template <typename CHARTYPE, std::size_t SIZE>
ts::UString& ts::UString::assign(const std::array<CHARTYPE, SIZE>& arr)
{
    return assign(arr, arr.size());
}


//----------------------------------------------------------------------------
// Template constructors.
//----------------------------------------------------------------------------

template <typename CHARTYPE, typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
ts::UString::UString(const std::vector<CHARTYPE>& vec, INT count, const allocator_type& alloc) :
    SuperClass(alloc)
{
    assign(vec, count);
}

template <typename CHARTYPE>
ts::UString::UString(const std::vector<CHARTYPE>& vec, const allocator_type& alloc) :
    SuperClass(alloc)
{
    assign(vec);
}

template <typename CHARTYPE, std::size_t SIZE, typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
ts::UString::UString(const std::array<CHARTYPE, SIZE>& arr, INT count, const allocator_type& alloc) :
    SuperClass(alloc)
{
    assign(arr, count);
}

template <typename CHARTYPE, std::size_t SIZE>
ts::UString::UString(const std::array<CHARTYPE, SIZE>& arr, const allocator_type& alloc) :
    SuperClass(alloc)
{
    assign(arr);
}


//----------------------------------------------------------------------------
// Split a string based on a separator character.
//----------------------------------------------------------------------------

template <class CONTAINER>
void ts::UString::splitAppend(CONTAINER& container, UChar separator, bool trimSpaces, bool removeEmpty) const
{
    const UChar* sep = nullptr;
    const UChar* input = data();
    const UChar* const end = data() + size();

    do {
        // Locate next separator
        for (sep = input; sep < end && *sep != separator; ++sep) {
        }
        // Extract segment
        UString segment(input, sep - input);
        if (trimSpaces) {
            segment.trim();
        }
        if (!removeEmpty || !segment.empty()) {
            container.push_back(segment);
        }
        // Move to beginning of next segment
        input = sep + 1;
    } while (sep < end);
}


//----------------------------------------------------------------------------
// Split the string into shell-style arguments.
//----------------------------------------------------------------------------

template <class CONTAINER>
void ts::UString::splitShellStyleAppend(CONTAINER& container) const
{
    const size_t end = this->size();
    size_t pos = 0;

    // Loop on all arguments.
    while (pos < end) {
        // Skip all spaces.
        while (pos < end && IsSpace(this->at(pos))) {
            pos++;
        }
        if (pos >= end) {
            break;
        }
        // Start of an argument.
        UString arg;
        UChar quote = 0;
        while (pos < end && (quote != 0 || !IsSpace(this->at(pos)))) {
            // Process opening and closing quotes.
            const UChar c = this->at(pos++);
            if (quote == 0 && (c == '"' || c == '\'')) {
                // Opening quote.
                quote = c;
            }
            else if (quote != 0 && c == quote) {
                // Closing quote.
                quote = 0;
            }
            else if (c == '\\' && pos < end) {
                // Get next character without interpretation.
                arg.append(this->at(pos++));
            }
            else {
                // Literal character.
                arg.append(c);
            }
        }
        // Argument completed.
        container.push_back(arg);
    }
}


//----------------------------------------------------------------------------
// Split a string into segments by starting / ending characters.
//----------------------------------------------------------------------------

template <class CONTAINER>
void ts::UString::splitBlocksAppend(CONTAINER& container, UChar startWith, UChar endWith, bool trimSpaces) const
{
    const UChar *sep = nullptr;
    const UChar* input = c_str();

    do {
        int blocksStillOpen = 0;
        // Locate next block-opening character
        while (*input != startWith && *input != 0) {
            // Input now points to the first block opening character
            ++input;
        }

        // Locate the next block-ending character corresponding to the considered block
        for (sep = input; *sep != 0; ++sep) {
            if (*sep == startWith) {
                ++blocksStillOpen;
                continue;
            }
            if (*sep == endWith) {
                --blocksStillOpen;
                if (blocksStillOpen == 0) {
                    break;
                }
            }

        }
        // Extract segment
        UString segment(input, sep - input + (*sep == 0 ? 0 : 1));
        // trim spaces if needed
        if (trimSpaces) {
            segment.trim();
        }
        container.push_back(segment);
        // Move to beginning of next segment
        input = *sep == 0 ? sep : sep + 1;
    } while (*sep != 0 && *(sep + 1) != 0);
}


//----------------------------------------------------------------------------
// Split a string into multiple lines which are not larger than a maximum.
//----------------------------------------------------------------------------

template <class CONTAINER>
void ts::UString::splitLinesAppend(CONTAINER& lines, size_t maxWidth, const UString& otherSeparators, const UString& nextMargin, bool forceSplit) const
{
    // If line smaller than max size or next margin too wide, return one line
    if (length() <= maxWidth || nextMargin.length() >= maxWidth) {
        lines.push_back(*this);
        return;
    }

    size_t marginLength = 0; // No margin on first line (supposed to be in str)
    size_t start = 0;        // Index in str of start of current line
    size_t eol = 0;          // Index in str of last possible end-of-line
    size_t cur = 0;          // Current index in str

    // Cut lines
    while (cur < length()) {
        // If @cur is a space or if the previous character is a possible separator, we may cut at cur.
        if (IsSpace(at(cur)) || (cur > start && otherSeparators.find(at(cur-1)) != NPOS)) {
            // Possible end of line here
            eol = cur;
        }
        // Determine if we need to cut here.
        bool cut = at(cur) == LINE_FEED;
        if (!cut && marginLength + cur - start >= maxWidth) { // Reached max width
            if (eol > start) {
                // Found a previous possible end-of-line
                cut = true;
            }
            else if (forceSplit) {
                // No previous possible end-of-line but force cut
                eol = cur;
                cut = true;
            }
        }
        // Perform line cut if necessary.
        if (cut) {
            // Add current line.
            UString line;
            if (marginLength > 0) {
                line.append(nextMargin);
            }
            line.append(substr(start, eol - start));
            line.trim(false, true); // trim trailing spaces
            lines.push_back(line);
            // Start new line, skip leading spaces
            marginLength = nextMargin.length();
            start = eol < length() && at(eol) == LINE_FEED ? eol + 1 : eol;
            while (start < length() && IsSpace(at(start)) && at(start) != LINE_FEED) {
                start++;
            }
            cur = eol = start;
        }
        else {
            cur++;
        }
    }

    // Rest of string on last line
    if (start < length()) {
        lines.push_back(nextMargin + substr(start));
    }
}


//----------------------------------------------------------------------------
// Join a part of a container of strings into one big string.
//----------------------------------------------------------------------------

template <class ITERATOR>
ts::UString ts::UString::Join(ITERATOR begin, ITERATOR end, const UString& separator, bool removeEmpty)
{
    UString res;
    while (begin != end) {
        if (!removeEmpty || !begin->empty()) {
            if (!res.empty()) {
                res.append(separator);
            }
            res.append(*begin);
        }
        ++begin;
    }
    return res;
}


//----------------------------------------------------------------------------
// Check if a container of strings contains something similar to this string.
//----------------------------------------------------------------------------

template <class CONTAINER>
bool ts::UString::isContainedSimilarIn(const CONTAINER& container) const
{
    for (const auto& it : container) {
        if (similar(it)) {
            return true;
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// Locate into a map an element with a similar string.
//----------------------------------------------------------------------------

template <class CONTAINER>
typename CONTAINER::const_iterator ts::UString::findSimilar(const CONTAINER& container) const
{
    typename CONTAINER::const_iterator it = container.begin();
    while (it != container.end() && !similar(it->first)) {
        ++it;
    }
    return it;
}


//----------------------------------------------------------------------------
// Save strings from a container into a file, one per line.
//----------------------------------------------------------------------------

template <class ITERATOR>
bool ts::UString::Save(ITERATOR begin, ITERATOR end, const UString& fileName, bool append)
{
    std::ofstream file(fileName.toUTF8().c_str(), append ? (std::ios::out | std::ios::app) : std::ios::out);
    Save(begin, end, file);
    file.close();
    return !file.fail();
}

template <class ITERATOR>
bool ts::UString::Save(ITERATOR begin, ITERATOR end, std::ostream& strm)
{
    while (strm && begin != end) {
        strm << *begin << std::endl;
        ++begin;
    }
    return !strm.fail();
}

template <class CONTAINER>
bool ts::UString::Save(const CONTAINER& container, std::ostream& strm)
{
    return Save(container.begin(), container.end(), strm);
}

template <class CONTAINER>
bool ts::UString::Save(const CONTAINER& container, const UString& fileName, bool append)
{
    return Save(container.begin(), container.end(), fileName, append);
}


//----------------------------------------------------------------------------
// Load strings from a file, one per line, and insert them in a container.
//----------------------------------------------------------------------------

template <class CONTAINER>
bool ts::UString::LoadAppend(CONTAINER& container, std::istream& strm)
{
    UString line;
    while (line.getLine(strm)) {
        container.push_back(line);
        // Weird behaviour (bug?) with gcc 4.8.5: When we read 2 consecutive lines
        // with the same length, the storage of the previous string *in the container*
        // if overwritten by the new line. Maybe not in all cases. No problem with
        // other versions or compilers. As a workaround, we clear the string
        // between read operations.
        line.clear();
    }
    return strm.eof();
}

template <class CONTAINER>
bool ts::UString::Load(CONTAINER& container, std::istream& strm)
{
    container.clear();
    return LoadAppend(container, strm);
}

template <class CONTAINER>
bool ts::UString::LoadAppend(CONTAINER& container, const UString& fileName)
{
    std::ifstream file(fileName.toUTF8().c_str());
    return LoadAppend(container, file);
}

template <class CONTAINER>
bool ts::UString::Load(CONTAINER& container, const UString& fileName)
{
    container.clear();
    return LoadAppend(container, fileName);
}


//----------------------------------------------------------------------------
// Convert a string into an integer.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
bool ts::UString::toInteger(INT& value, const UString& thousandSeparators, size_type decimals, const UString& decimalSeparators, INT minValue, INT maxValue) const
{
    // Locate actual begin and end of integer value. Skip leading redundant '+' sign.
    const UChar* start = data();
    const UChar* end = start + length();
    while (start < end && (IsSpace(*start) || *start == u'+')) {
        ++start;
    }
    while (start < end && IsSpace(*(end-1))) {
        --end;
    }

    // Decode the value. Use unsigned or signed version.
    return ToIntegerHelper(start, end, value, thousandSeparators, decimals, decimalSeparators) && value >= minValue && value <= maxValue;
}


//----------------------------------------------------------------------------
// Internal helper for toInteger, unsigned version.
//----------------------------------------------------------------------------

template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type*>
bool ts::UString::ToIntegerHelper(const UChar* start, const UChar* end, INT& value, const UString& thousandSeparators, size_type decimals, const UString& decimalSeparators)
{
    // Initial value, up to decode error.
    value = static_cast<INT>(0);

    // Look for hexadecimal prefix.
    int base = 10;
    if (start + 1 < end && start[0] == u'0' && (start[1] == u'x' || start[1] == u'X')) {
        start += 2;
        base = 16;
    }

    // Filter empty string.
    if (start >= end) {
        return false;
    }

    // Decimal digits handling.
    bool dec_found = false;   // a decimal point was found
    size_type dec_count = 0;  // number of decimal digits found

    // Decode the string.
    while (start < end) {
        const int digit = ToDigit(*start, base);
        if (digit >= 0) {
            // Character is a valid digit. Ignore extraneous decimal digits.
            if (!dec_found || dec_count < decimals) {
                value = value * static_cast<INT>(base) + static_cast<INT>(digit);
            }
            // Count decimal digits, after the decimal point.
            if (dec_found) {
                ++dec_count;
            }
        }
        else if (decimalSeparators.contain(*start)) {
            // Found a decimal point. Only one is allowed.
            // A decimal point is allowed only in base 10.
            if (dec_found || base != 10) {
                return false;
            }
            dec_found = true;
        }
        else if (!thousandSeparators.contain(*start)) {
            // Character is not a possible thousands separator to ignore.
            return false;
        }
        ++start;
    }

    // If decimals are missing, adjust the value.
    while (dec_count < decimals) {
        value = 10 * value;
        ++dec_count;
    }

    return true;
}


//----------------------------------------------------------------------------
// Internal helper for toInteger, signed version.
//----------------------------------------------------------------------------

template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type*>
bool ts::UString::ToIntegerHelper(const UChar* start, const UChar* end, INT& value, const UString& thousandSeparators, size_type decimals, const UString& decimalSeparators)
{
    // Skip optional minus sign.
    bool negative = false;
    if (start < end && *start == '-') {
        ++start;
        negative = true;
    }

    // Decode the string as an unsigned integer.
    typename std::make_unsigned<INT>::type uvalue = 0;
    const bool ok = ToIntegerHelper(start, end, uvalue, thousandSeparators, decimals, decimalSeparators);

    // Convert the unsigned integer as signed integer with the appropriate sign.
    value = static_cast<INT>(uvalue);
    if (negative) {
        value = -value;
    }

    return ok;
}


//----------------------------------------------------------------------------
// Convert a string containing a list of integers into a container of integers.
//----------------------------------------------------------------------------

template <class CONTAINER, typename std::enable_if<std::is_integral<typename CONTAINER::value_type>::value>::type*>
bool ts::UString::toIntegers(CONTAINER& container,
                             const UString& thousandSeparators,
                             const UString& listSeparators,
                             size_type decimals,
                             const UString& decimalSeparators,
                             typename CONTAINER::value_type minValue,
                             typename CONTAINER::value_type maxValue) const
{
    // Let's name int_type the integer type.
    // In all STL standard containers, value_type is a typedef for the element type.
    typedef typename CONTAINER::value_type int_type;

    // Reset the content of the container
    container.clear();

    // Locate segments in the string
    size_type start = 0;
    size_type farEnd = length();

    // Loop on segments
    while (start < farEnd) {
        // Skip spaces and list separators
        while (start < farEnd && (IsSpace((*this)[start]) || listSeparators.find((*this)[start]) != NPOS)) {
            ++start;
        }
        // Locate end of segment
        size_type end = start;
        while (end < farEnd && listSeparators.find((*this)[end]) == NPOS) {
            ++end;
        }
        // Exit at end of string
        if (start >= farEnd) {
            break;
        }
        // Decode segment
        int_type value = static_cast<int_type>(0);
        if (!substr(start, end - start).toInteger<int_type>(value, thousandSeparators, decimals, decimalSeparators, minValue, maxValue)) {
            return false;
        }
        container.push_back(value);

        // Move to next segment
        start = end;
    }

    return true;
}


//----------------------------------------------------------------------------
// Convert a string into a floating-point.
//----------------------------------------------------------------------------

template <typename FLT, typename std::enable_if<std::is_floating_point<FLT>::value>::type*>
bool ts::UString::toFloat(FLT& value, FLT minValue, FLT maxValue) const
{
    // Convert to an 8-bit string.
    std::string str;
    toTrimmed().toUTF8(str);

    // Use a good old scanf to decode the value.
    // Use an additional dummy character to make sure there is nothing more to read.
    double flt = 0.0;
    char dummy = 0;
    const int count = ::sscanf(str.c_str(), "%lf%c", &flt, &dummy);
    value = FLT(flt);
    return count == 1 && value >= minValue && value <= maxValue;
}


//----------------------------------------------------------------------------
// Append an array of C-strings to a container of strings.
//----------------------------------------------------------------------------

template <class CONTAINER>
CONTAINER& ts::UString::Append(CONTAINER& container, int argc, const char* const argv[])
{
    const size_type size = argc < 0 ? 0 : size_type(argc);
    for (size_type i = 0; i < size; ++i) {
        container.push_back(UString::FromUTF8(argv[i]));
    }
    return container;
}


//----------------------------------------------------------------------------
// Format a string containing a decimal value.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
ts::UString ts::UString::Decimal(INT value,
                                 size_type min_width,
                                 bool right_justified,
                                 const UString& separator,
                                 bool force_sign,
                                 UChar pad)
{
    // We build the result string in s.
    UString s;
    DecimalHelper(s, value, separator, force_sign);

    // Adjust string width.
    if (s.size() < min_width) {
        if (right_justified) {
            s.insert(0, min_width - s.size(), pad);
        }
        else {
            s.append(min_width - s.size(), pad);
        }
    }

    // Return the formatted result
    return s;
}


//----------------------------------------------------------------------------
// Format a string containing a list of decimal values.
//----------------------------------------------------------------------------

template <class CONTAINER, typename std::enable_if<std::is_integral<typename CONTAINER::value_type>::value>::type*>
ts::UString ts::UString::Decimal(const CONTAINER& values, const UString& separator, bool force_sign)
{
    UString result;
    for (const typename CONTAINER::value_type& val : values) {
        UString s;
        DecimalHelper(s, val, u"", force_sign);
        if (!result.empty()) {
            result.append(separator);
        }
        result.append(s);
    }
    return result;
}


//----------------------------------------------------------------------------
// Internal helpers for Decimal(), unsigned version.
//----------------------------------------------------------------------------

template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_unsigned<INT>::value>::type*>
void ts::UString::DecimalHelper(UString& result, INT value, const UString& separator, bool force_sign)
{
    // Avoid reallocating (most of the time).
    result.clear();
    result.reserve(32);

    // We build the result string IN REVERSE ORDER
    // So, we need the separator in reverse order too.
    UString sep(separator);
    sep.reverse();

    // Format the value
    int count = 0;
    do {
        result.push_back(u'0' + UChar(value % 10));
        value /= 10;
        if (++count % 3 == 0 && value != 0) {
            result.append(sep);
        }
    } while (value != 0);
    if (force_sign) {
        result.push_back(u'+');
    }

    // Reverse characters in the string
    result.reverse();
}


//----------------------------------------------------------------------------
// Internal helpers for Decimal(), signed version.
//----------------------------------------------------------------------------

template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value>::type*>
void ts::UString::DecimalHelper(UString& result, INT value, const UString& separator, bool force_sign)
{
    // Unsigned version of the signed type (same size).
    typedef typename std::make_unsigned<INT>::type UNSINT;

    if (value == std::numeric_limits<INT>::min()) {
        DecimalMostNegative<INT>(result, separator);
    }
    else if (value < 0) {
        DecimalHelper<UNSINT>(result, static_cast<UNSINT>(-value), separator, false);
        result.insert(0, 1, u'-');
    }
    else {
        DecimalHelper<UNSINT>(result, static_cast<UNSINT>(value), separator, force_sign);
    }
}


//----------------------------------------------------------------------------
// Internal helper for Decimal() when the value is the most negative value of
// a signed type (cannot be made positive inside the same signed type).
//----------------------------------------------------------------------------

template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value && sizeof(INT) == 8>::type*>
void ts::UString::DecimalMostNegative(UString& result, const UString& separator)
{
    // Specialization for 64-bit signed type to avoid infinite recursion.
    // Hard-coded value since there is no way to build it:
    result = u"-9223372036854775808";
    if (!separator.empty()) {
        int count = 0;
        for (size_t i = result.size() - 1; i > 0; --i) {
            if (++count % 3 == 0) {
                result.insert(i, separator);
            }
        }
    }
}

template<typename INT, typename std::enable_if<std::is_integral<INT>::value && std::is_signed<INT>::value && sizeof(INT) < 8>::type*>
void ts::UString::DecimalMostNegative(UString& result, const UString& separator)
{
    // INT is less than 64-bit long. Use an intermediate 64-bit conversion to have a valid positive value.
    DecimalHelper<int64_t>(result, static_cast<int64_t>(std::numeric_limits<INT>::min()), separator, false);
}


//----------------------------------------------------------------------------
// Format a string containing an hexadecimal value.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
ts::UString ts::UString::Hexa(INT svalue,
                              size_type width,
                              const UString& separator,
                              bool use_prefix,
                              bool use_upper)
{
    // We build the result string in s IN REVERSE ORDER
    UString s;
    s.reserve(32); // avoid reallocating (most of the time)

    // So, we need the separator in reverse order too.
    UString sep(separator);
    sep.reverse();

    // Default to the natural size of the type.
    if (width == 0) {
        width = 2 * sizeof(INT);
    }

    // In hexadecimal, always format the unsigned version of the binary value.
    typedef typename std::make_unsigned<INT>::type UNSINT;
    UNSINT value = static_cast<UNSINT>(svalue);

    // Format the value
    int count = 0;
    while (width != 0) {
        const int nibble = int(value & 0xF);
        value >>= 4;
        --width;
        if (nibble < 10) {
            s.push_back(u'0' + UChar(nibble));
        }
        else if (use_upper) {
            s.push_back(u'A' + UChar(nibble - 10));
        }
        else {
            s.push_back(u'a' + UChar(nibble - 10));
        }
        if (++count % 4 == 0 && width > 0) {
            s += sep;
        }
    }

    // Add the optional prefix, still in reverse order.
    if (use_prefix) {
        s.push_back(u'x');
        s.push_back(u'0');
    }

    // Reverse characters in string
    return s.toReversed();
}


//----------------------------------------------------------------------------
// Format a string containing an hexadecimal value (variant).
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
ts::UString ts::UString::HexaMin(INT svalue,
                                 size_type min_width,
                                 const UString& separator,
                                 bool use_prefix,
                                 bool use_upper)
{
    // We build the result string in s IN REVERSE ORDER
    UString s;
    s.reserve(32); // avoid reallocating (most of the time)

    // So, we need the separator in reverse order too.
    UString sep(separator);
    sep.reverse();

    // Minimum number of hexa digits to format.
    size_type min_digit = min_width > 0 ? 0 : 2 * sizeof(INT);

    // Remove prefix width from the minimum width.
    if (use_prefix && min_width >= 2) {
        min_width -= 2;
    }

    // In hexadecimal, always format the unsigned version of the binary value.
    typedef typename std::make_unsigned<INT>::type UNSINT;
    UNSINT value = static_cast<UNSINT>(svalue);

    // Format the value
    for (size_type digit_count = 0; digit_count == 0 || digit_count < min_digit || s.size() < min_width || value != 0; digit_count++) {
        const int nibble = int(value & 0xF);
        value >>= 4;
        if (digit_count % 4 == 0 && digit_count > 0) {
            s += sep;
        }
        if (nibble < 10) {
            s.push_back(u'0' + UChar(nibble));
        }
        else if (use_upper) {
            s.push_back(u'A' + UChar(nibble - 10));
        }
        else {
            s.push_back(u'a' + UChar(nibble - 10));
        }
    }

    // Add the optional prefix, still in reverse order.
    if (use_prefix) {
        s.push_back(u'x');
        s.push_back(u'0');
    }

    // Reverse characters in string
    return s.toReversed();
}


//----------------------------------------------------------------------------
// Format a percentage string.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
ts::UString ts::UString::Percentage(INT value, INT total)
{
    if (total < 0) {
        return u"?";
    }
    if (total == 0) {
        return u"0.00%";
    }
    else {
        // Integral percentage
        const int p1 = int((100 * uint64_t(value)) / uint64_t(total));
        // Percentage first 2 decimals
        const int p2 = int(((10000 * uint64_t(value)) / uint64_t(total)) % 100);
        return Format(u"%d.%02d%%", {p1, p2});
    }
}


//----------------------------------------------------------------------------
// Reduce the size of the string to a given length from an alien integer type.
//----------------------------------------------------------------------------

template <typename INT, typename std::enable_if<std::is_integral<INT>::value>::type*>
void ts::UString::trimLength(INT length, bool trimTrailingSpaces)
{
    // We assume here that UString::size_type is the largest unsigned type
    // and that it is safe to convert any positive value into this type.
    resize(std::min<size_type>(size(), size_type(std::max<INT>(0, length))));
    trim(false, trimTrailingSpaces);
}


//----------------------------------------------------------------------------
// Convert a container of strings into one big string where all elements are
// properly quoted when necessary.
//----------------------------------------------------------------------------

template <class CONTAINER>
void ts::UString::quotedLine(const CONTAINER& container, UChar quoteCharacter, const UString& specialCharacters)
{
    clear();
    for (const auto& it : container) {
        if (!empty()) {
            append(SPACE);
        }
        append(it.toQuoted(quoteCharacter, specialCharacters));
    }
}

template <class CONTAINER>
ts::UString ts::UString::ToQuotedLine(const CONTAINER& container, UChar quoteCharacter, const UString& specialCharacters)
{
    UString result;
    result.quotedLine(container, quoteCharacter, specialCharacters);
    return result;
}


//----------------------------------------------------------------------------
// Split this string in space-separated possibly-quoted elements.
//----------------------------------------------------------------------------

template <class CONTAINER>
void ts::UString::fromQuotedLine(CONTAINER& container, const UString& quoteCharacters, const UString& specialCharacters) const
{
    container.clear();

    // Loop on words.
    size_type index = 0;
    while (index < size()) {

        // Skip spaces before next word.
        while (index < size() && IsSpace(at(index))) {
            ++index;
        }

        // Return when no more word is available.
        if (index >= size()) {
            return;
        }

        // Current word under construction.
        UString word;
        UChar quoteChar = CHAR_NULL;
        bool quoteOpen = false;

        // Accumulate characters from the current word.
        while (index < size() && (quoteOpen || !IsSpace(at(index)))) {
            UChar c = at(index++);
            if (!quoteOpen && quoteCharacters.contain(c)) {
                // Start of a quoted sequence.
                quoteOpen = true;
                quoteChar = c;
            }
            else if (quoteOpen && c == quoteChar) {
                // End of quoted sequence.
                quoteOpen = false;
            }
            else if (c == '\\' && index < size()) {
                // Start of an escape sequence.
                c = at(index++);
                switch (c) {
                    case u'b': c = BACKSPACE; break;
                    case u'f': c = FORM_FEED; break;
                    case u'n': c = LINE_FEED; break;
                    case u'r': c = CARRIAGE_RETURN; break;
                    case u't': c = HORIZONTAL_TABULATION; break;
                    default: break;
                }
                word.push_back(c);
            }
            else {
                // Just a regular character.
                word.push_back(c);
            }
        }

        // End of word, push it.
        container.push_back(word);
    }
}
