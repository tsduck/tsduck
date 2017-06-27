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
//  Build a string containing the hexa/ascii dump of a memory area
//
//----------------------------------------------------------------------------

#include "tsHexa.h"
#include "tsFormat.h"


//----------------------------------------------------------------------------
// Build a string containing the hexa dump of (data, size).
// Each line is indented by 'indent' space chars. The maximum amount
// of chars per line is limited by line_width. If OFFSET is specified,
// the first offset value to display is 'init_offset'.
//----------------------------------------------------------------------------

std::string ts::Hexa(const void *data,
                     size_t size,
                     uint32_t flags,
                     size_t indent,
                     size_t line_width,
                     size_t init_offset)
{
    std::string s;
    AppendHexa(s, data, size, flags, indent, line_width, init_offset);
    return s;
}


//----------------------------------------------------------------------------
// Same with a string
//----------------------------------------------------------------------------

std::string ts::Hexa(const std::string& str,
                     uint32_t flags,
                     size_t indent,
                     size_t line_width,
                     size_t init_offset)
{
    std::string s;
    AppendHexa(s, str.c_str(), str.length(), flags, indent, line_width, init_offset);
    return s;
}


//----------------------------------------------------------------------------
// Same with a ByteBlock
//----------------------------------------------------------------------------

std::string ts::Hexa(const ByteBlock& bb,
                     uint32_t flags,
                     size_t indent,
                     size_t line_width,
                     size_t init_offset)
{
    std::string s;
    AppendHexa(s, bb.data(), bb.size(), flags, indent, line_width, init_offset);
    return s;
}


//----------------------------------------------------------------------------
// Add more dump to a string.
// Return a reference to the input string.
//----------------------------------------------------------------------------

std::string& ts::AppendHexa(std::string& str,
                            const void *data,
                            size_t size,
                            uint32_t flags,
                            size_t indent,
                            size_t line_width,
                            size_t init_offset)
{
    const uint8_t* raw(static_cast <const uint8_t*> (data));

    // Make sure we have something to display (default is hexa)
    if ((flags & (hexa::HEXA | hexa::C_STYLE | hexa::BINARY | hexa::BIN_NIBBLE | hexa::ASCII)) == 0) {
        flags |= hexa::HEXA;
    }

    // Width of an hexa byte: "XX" (2) or "0xXX," (5)
    size_t hexa_width;
    const char* byte_prefix;
    const char* byte_suffix;

    if (flags & hexa::C_STYLE) {
        hexa_width  = 5;
        byte_prefix = "0x";
        byte_suffix = ",";
        flags |= hexa::HEXA; // Enforce hexa flag
    }
    else if (flags & (hexa::HEXA | hexa::SINGLE_LINE)) {
        hexa_width  = 2;
        byte_prefix = "";
        byte_suffix = "";
    }
    else {
        hexa_width  = 0;
        byte_prefix = "";
        byte_suffix = "";
    }

    // Specific case: simple dump, everything on one line.
    if (flags & hexa::SINGLE_LINE) {
        str.reserve (str.length() + (hexa_width + 1) * size);
        for (size_t i = 0; i < size; ++i) {
            if (i > 0) {
                str.append (1, ' ');
            }
            str.append (byte_prefix);
            str.append (Format ("%02X", int (raw [i])));
            str.append (byte_suffix);
        }
        return str;
    }

    // Width of offset field
    size_t offset_width;

    if ((flags & hexa::OFFSET) == 0) {
        offset_width = 0;
    }
    else if (flags & hexa::WIDE_OFFSET) {
        offset_width = 8;
    }
    else if (init_offset + size <= 0x10000) {
        offset_width = 4;
    }
    else {
        offset_width = 8;
    }

    // Width of a binary byte
    size_t bin_width;

    if (flags & hexa::BIN_NIBBLE) {
        bin_width = 9;
        flags |= hexa::BINARY;  // Enforce binary flag
    }
    else if (flags & hexa::BINARY) {
        bin_width = 8;
    }
    else {
        bin_width = 0;
    }

    // Pre-allocation to avoid too frequent reallocations.
    str.reserve (str.length() + (hexa_width + bin_width + 5) * size);

    // Number of non-byte characters
    size_t add_width = indent;
    if (offset_width != 0) {
        add_width += offset_width + 3;
    }
    if ((flags & hexa::HEXA) && (flags & (hexa::BINARY | hexa::ASCII))) {
        add_width += 2;
    }
    if ((flags & hexa::BINARY) && (flags & hexa::ASCII)) {
        add_width += 2;
    }

    // Computes max number of dumped bytes per line
    size_t bytes_per_line;

    if (flags & hexa::BPL) {
        bytes_per_line = line_width;
    }
    else if (add_width >= line_width) {
        bytes_per_line = 8;  // arbitrary, if indent is too long
    }
    else {
        bytes_per_line = (line_width - add_width) /
            (((flags & hexa::HEXA) ? (hexa_width + 1) : 0) +
             ((flags & hexa::BINARY) ? (bin_width + 1) : 0) +
             ((flags & hexa::ASCII) ? 1 : 0));
        if (bytes_per_line > 1) {
            bytes_per_line = bytes_per_line & ~1; // force even value
        }
    }
    if (bytes_per_line == 0) {
        bytes_per_line = 8;  // arbitrary, if ended up with none
    }

    // Display data
    for (size_t line = 0; line < size; line += bytes_per_line) {

        // Number of bytes on this line (last line may be shorter)
        size_t line_size = line + bytes_per_line <= size ? bytes_per_line : size - line;

        // Beginning of line
        str.append (indent, ' ');
        if (flags & hexa::OFFSET) {
            str.append (Format ("%0*" FMT_SIZE_T "X:  ", int (offset_width), init_offset + line));
        }

        // Hexa dump
        if (flags & hexa::HEXA) {
            for (size_t byte = 0; byte < line_size; byte++) {
                str.append (byte_prefix);
                str.append (Format ("%02X", int (raw [line + byte])));
                str.append (byte_suffix);
                if (byte < bytes_per_line - 1) {
                    str.append (1, ' ');
                }
            }
            if (flags & (hexa::BINARY | hexa::ASCII)) { // more to come
                if (line_size < bytes_per_line) {
                    str.append ((hexa_width + 1) * (bytes_per_line - line_size) - 1, ' ');
                }
                str.append (2, ' ');
            }
        }

        // Binary dump
        if (flags & hexa::BINARY) {
            for (size_t byte = 0; byte < line_size; byte++) {
                int b = int (raw [line + byte]);
                for (int i = 7; i >= 0; i--) {
                    str.append (1, '0' + ((b >> i) & 0x01));
                    if (i == 4 && (flags & hexa::BIN_NIBBLE)) {
                        str.append (1, '.');
                    }
                }
                if (byte < bytes_per_line - 1) {
                    str.append (1, ' ');
                }
            }
            if (flags & hexa::ASCII) { // more to come
                if (line_size < bytes_per_line) {
                    str.append ((bin_width + 1) * (bytes_per_line - line_size) - 1, ' ');
                }
                str.append (2, ' ');
            }
        }

        // ASCII dump
        if (flags & hexa::ASCII) {
            for (size_t byte = 0; byte < line_size; byte++) {
                // Make sure european characters are detected as printable,
                // even if isprint(3) does not.
                uint8_t c = raw [line + byte];
                bool printable = isprint (c) || c >= 0xA0;
                str.push_back (printable ? c : '.');
            }
        }
        str.push_back ('\n');
    }

    return str;
}
