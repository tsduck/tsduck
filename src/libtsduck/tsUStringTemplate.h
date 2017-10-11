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
