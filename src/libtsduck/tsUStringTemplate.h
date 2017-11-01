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

#pragma once


//----------------------------------------------------------------------------
// Split a string based on a separator character.
//----------------------------------------------------------------------------

template <class CONTAINER>
void ts::UString::split(CONTAINER& container, UChar separator, bool trimSpaces) const
{
    const UChar* sep = 0;
    const UChar* input = c_str();
    container.clear();

    do {
        // Locate next separator
        for (sep = input; *sep != separator && *sep != 0; ++sep) {
        }
        // Extract segment
        UString segment(input, sep - input);
        if (trimSpaces) {
            segment.trim();
        }
        container.push_back(segment);
        // Move to beginning of next segment
        input = *sep == 0 ? sep : sep + 1;
    } while (*sep != 0);
}


//----------------------------------------------------------------------------
// Split a string into segments by starting / ending characters.
//----------------------------------------------------------------------------

template <class CONTAINER>
void ts::UString::splitBlocks(CONTAINER& container, UChar startWith, UChar endWith, bool trimSpaces) const
{
    const UChar *sep = 0;
    const UChar* input = c_str();
    container.clear();

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
void ts::UString::splitLines(CONTAINER& lines, size_t maxWidth, const UString& otherSeparators, const UString& nextMargin, bool forceSplit) const
{
    // Cleanup container
    lines.clear();

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
        if (IsSpace(at(cur)) || (cur > start && otherSeparators.find(at(cur-1)) != NPOS)) {
            // Possible end of line here
            eol = cur;
        }
        bool cut = false;
        if (marginLength + cur - start >= maxWidth) { // Reached max width
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
        if (cut) {
            lines.push_back((marginLength == 0 ? UString() : nextMargin) + substr(start, eol - start));
            marginLength = nextMargin.length();
            // Start new line, skip leading spaces
            start = eol;
            while (start < length() && IsSpace(at(start))) {
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
ts::UString ts::UString::Join(ITERATOR begin, ITERATOR end, const UString& separator)
{
    UString res;
    while (begin != end) {
        if (!res.empty()) {
            res.append(separator);
        }
        res.append(*begin);
        ++begin;
    }
    return res;
}


//----------------------------------------------------------------------------
// Check if a container of strings contains something similar to this string.
//----------------------------------------------------------------------------

template <class CONTAINER>
bool ts::UString::containSimilar(const CONTAINER& container) const
{
    for (typename CONTAINER::const_iterator it = container.begin(); it != container.end(); ++it) {
        if (similar(*it)) {
            return true;
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// Locate into a map an element with a similar string.
//----------------------------------------------------------------------------

template <class CONTAINER>
typename CONTAINER::const_iterator ts::UString::findSimilar(const CONTAINER& container)
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
bool ts::UString::Save(ITERATOR begin, ITERATOR end, const std::string& fileName, bool append)
{
    std::ofstream file(fileName.c_str(), append ? (std::ios::out | std::ios::app) : std::ios::out);
    while (file && begin != end) {
        file << *begin << std::endl;
        ++begin;
    }
    file.close();
    return !file.fail();
}


//----------------------------------------------------------------------------
// Save strings from a container into a file, one per line.
//----------------------------------------------------------------------------

template <class CONTAINER>
bool ts::UString::Save(const CONTAINER& container, const std::string& fileName, bool append)
{
    return Save(container.begin(), container.end(), fileName, append);
}


//----------------------------------------------------------------------------
// Load strings from a file, one per line, and insert them in a container.
//----------------------------------------------------------------------------

template <class CONTAINER>
bool ts::UString::LoadAppend(CONTAINER& container, const std::string& fileName)
{
    UString line;
    std::ifstream file(fileName.c_str());
    while (line.getLine(file)) {
        container.push_back(line);
    }
    return file.eof();
}


//----------------------------------------------------------------------------
// Load strings from a file, one per line, into a container.
//----------------------------------------------------------------------------

template <class CONTAINER>
bool ts::UString::Load(CONTAINER& container, const std::string& fileName)
{
    container.clear();
    return LoadAppend(container, fileName);
}


//----------------------------------------------------------------------------
// Convert a string into an integer.
//----------------------------------------------------------------------------

template <typename INT>
bool ts::UString::toInteger(INT& value, const UString& thousandSeparators) const
{
    // In this function, we work on formal integer types INT. We use std::numeric_limits<INT> to test the
    // capabilities of the type (is_signed, etc.) But, for each instantiation of INT, some expression
    // may not make sense and the Microsoft compiler complains about that. Disable specific warnings
#if defined(TS_MSC)
#pragma warning(push)
#pragma warning(disable:4127)
#pragma warning(disable:4146)
#endif

    typedef typename std::numeric_limits<INT> limits;

    // Initial value, up to decode error
    value = static_cast<INT>(0);

    // Reject non-integer type (floating points, etc.) and invalid parameters
    if (!limits::is_integer) {
        return false;
    }

    // Locate actual begin and end of integer value
    const UChar* start = data();
    const UChar* end = start + length();
    while (start < end && IsSpace(*start)) {
        ++start;
    }
    while (start < end && IsSpace(*(end-1))) {
        --end;
    }

    // Skip optional sign
    bool negative = false;
    if (start < end) {
        if (*start == '+') {
            ++start;
        }
        else if (*start == '-') {
            if (!limits::is_signed) {
                // INT type is unsigned, invalid signed value
                return false;
            }
            ++start;
            negative = true;
        }
    }

    // Look for hexadecimal prefix
    int base = 10;
    if (start + 1 < end && start[0] == UChar('0') && (start[1] == UChar('x') || start[1] == UChar('X'))) {
        start += 2;
        base = 16;
    }

    // Filter empty string
    if (start == end) {
        return false;
    }
    assert(start < end);

    // Decode the string
    while (start < end) {
        const int digit = ToDigit(*start, base);
        if (digit >= 0) {
            // Character is a valid digit
            value = value * static_cast<INT>(base) + static_cast<INT>(digit);
        }
        else if (thousandSeparators.find(*start) == NPOS) {
            // Character is not a possible thousands separator
            break;
        }
        ++start;
    }

    // Apply sign
    if (negative) {
        value = -value;
    }

    // Success only if we went down to the end of string
    return start == end;

#if defined(TS_MSC)
#pragma warning(pop)
#endif
}


//----------------------------------------------------------------------------
// Convert a string containing a list of integers into a container of integers.
//----------------------------------------------------------------------------

template <class CONTAINER>
bool ts::UString::toIntegers(CONTAINER& container, const UString& thousandSeparators, const UString& listSeparators) const
{
    // Let's name INT the integer type.
    // In all STL standard containers, value_type is a typedef for the element type.
    typedef typename CONTAINER::value_type INT;

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
        INT value = static_cast<INT>(0);
        if (!substr(start, end - start).toInteger<INT>(value, thousandSeparators)) {
            return false;
        }
        container.push_back(value);

        // Move to next segment
        start = end;
    }

    return true;
}


//----------------------------------------------------------------------------
// Append an array of C-strings to a container of strings.
//----------------------------------------------------------------------------

template <class CONTAINER>
CONTAINER& ts::UString::Append(CONTAINER& container, int argc, const char* const argv[])
{
    const size_type size = argc < 0 ? 0 : size_type(argc);
    for (size_type i = 0; i < size; ++i) {
        container.push_back(UString(argv[i]));
    }
    return container;
}
