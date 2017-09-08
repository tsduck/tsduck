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
// Split a string based on a separator character (comma by default).
//----------------------------------------------------------------------------

template <class CONTAINER>
CONTAINER& ts::SplitString(CONTAINER& container, const char *input, char separator, bool trimSpaces)
{
    const char* sep;
    container.clear();

    do {
        // Locate next separator
        for (sep = input; *sep != separator && *sep != 0; ++sep) {
        }
        // Extract segment
        std::string segment(input, sep - input);
        if (trimSpaces) {
            ts::Trim(segment);
        }
        container.push_back(segment);
        // Move to beginning of next segment
        input = *sep == 0 ? sep : sep + 1;
    } while (*sep != 0);

    return container;
}


//----------------------------------------------------------------------------
// Split a string into segments which are identified by their starting /
// ending characters (respectively "[" and "]" by default).
//----------------------------------------------------------------------------

template <class CONTAINER>
CONTAINER& ts::SplitBlocks(CONTAINER& container, const char* input, char startWith, char endWith, bool trimSpaces)
{
    const char *sep;
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
        std::string segment(input, sep - input + (*sep == 0 ? 0 : 1));
        // trim spaces if needed
        if (trimSpaces) {
            ts::Trim(segment);
        }
        container.push_back(segment);
        // Move to beginning of next segment
        input = *sep == 0 ? sep : sep + 1;
    } while (*sep != 0 && *(sep + 1) != 0);

    return container;
}


//----------------------------------------------------------------------------
// Join a part of a container of strings into one big string.
//----------------------------------------------------------------------------

template <class ITERATOR>
std::string ts::JoinStrings(ITERATOR begin, ITERATOR end, const std::string& separator)
{
    std::string res;
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
// Split a string into multiple lines which are not larger than a specified maximum width.
//----------------------------------------------------------------------------

template <class CONTAINER>
CONTAINER& ts::SplitLines(CONTAINER& lines,
                          const std::string& str,
                          size_t maxWidth,
                          const std::string& otherSeparators,
                          const std::string& nextMargin,
                          bool forceSplit)
{
    // Cleanup container
    lines.clear();

    // If line smaller than max size or next margin too wide, return one line
    if (str.length() <= maxWidth || nextMargin.length() >= maxWidth) {
        lines.push_back (str);
        return lines;
    }

    size_t marginLength = 0; // No margin on first line (supposed to be in str)
    size_t start = 0;        // Index in str of start of current line
    size_t eol = 0;          // Index in str of last possible end-of-line
    size_t cur = 0;          // Current index in str

    // Cut lines
    while (cur < str.length()) {
        if (std::isspace(str[cur]) || (cur > start && otherSeparators.find (str[cur-1]) != std::string::npos)) {
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
            lines.push_back((marginLength == 0 ? "" : nextMargin) + str.substr(start, eol - start));
            marginLength = nextMargin.length();
            // Start new line, skip leading spaces
            start = eol;
            while (start < str.length() && std::isspace(str[start])) {
                start++;
            }
            cur = eol = start;
        }
        else {
            cur++;
        }
    }

    // Rest of string on last line
    if (start < str.length()) {
        lines.push_back(nextMargin + str.substr(start));
    }

    return lines;
}


//----------------------------------------------------------------------------
// Check if a container of strings contains something similar to a string.
//----------------------------------------------------------------------------

template <class CONTAINER>
bool ts::ContainSimilarString(const CONTAINER& container, const std::string& str)
{
    for (typename CONTAINER::const_iterator it = container.begin(); it != container.end(); ++it) {
        if (SimilarStrings(*it, str)) {
            return true;
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// Append an array of C-strings to a container of strings.
// Return a reference to the container.
//----------------------------------------------------------------------------

template <class CONTAINER>
CONTAINER& ts::AppendContainer(CONTAINER& container, int argc, const char* const argv[])
{
    const size_t size = argc < 0 ? 0 : size_t(argc);
    for (size_t i = 0; i < size; ++i) {
        container.push_back(argv[i]);
    }
    return container;
}


//----------------------------------------------------------------------------
// Load strings from a file, one per line, and append them at the end of a container.
//----------------------------------------------------------------------------

template <class CONTAINER>
bool ts::LoadAppendStrings(CONTAINER& container, const std::string& fileName)
{
    size_t len;
    std::string line;
    std::ifstream file(fileName.c_str());

    while (std::getline(file, line)) {
        // Remove potential trailing mixed CR/LF characters
        for (len = line.size(); len > 0 && (line[len-1] == '\r' || line[len-1] == '\n'); --len) {
        }
        line.resize(len);
        container.push_back(line);
    }

    return file.eof();
}


//----------------------------------------------------------------------------
// Load strings from a file, one per line, into a container.
//----------------------------------------------------------------------------

template <class CONTAINER>
bool ts::LoadStrings(CONTAINER& container, const std::string& fileName)
{
    container.clear();
    return LoadAppendStrings(container, fileName);
}


//----------------------------------------------------------------------------
// Save strings from a container into a file, one per line.
//----------------------------------------------------------------------------

template <class ITERATOR>
bool ts::SaveStrings(ITERATOR begin, ITERATOR end, const std::string& fileName, bool append)
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
bool ts::SaveStrings(const CONTAINER& container, const std::string& fileName, bool append)
{
    return SaveStrings(container.begin(), container.end(), fileName, append);
}


//----------------------------------------------------------------------------
// Get the length of the longest string in a container of strings.
//----------------------------------------------------------------------------

template <class CONTAINER>
size_t ts::LargestLength(const CONTAINER& container)
{
    size_t largest = 0;
    for (typename CONTAINER::const_iterator it = container.begin(); it != container.end(); ++it) {
        largest = std::max(largest, it->length());
    }
    return largest;
}


//----------------------------------------------------------------------------
// Locate into a map an element with a similar string.
//----------------------------------------------------------------------------

template <class CONTAINER>
typename CONTAINER::const_iterator ts::FindSimilar(const CONTAINER& container, const std::string& key)
{
    typename CONTAINER::const_iterator it = container.begin();
    while (it != container.end() && !SimilarStrings(key, it->first)) {
        ++it;
    }
    return it;
}
