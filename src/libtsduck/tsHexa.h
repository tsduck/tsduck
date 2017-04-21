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
//!
//!  @file
//!  Build a string containing the hexa/ascii dump of a memory area
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsByteBlock.h"

namespace ts {

    //!
    //! Definitions for the ts::Hexa() family of functions.
    //!
    namespace hexa {

        const size_t DEFAULT_LINE_WIDTH = 78;

        enum Flags {
            HEXA        = 0x0001,  // Dump hexa values
            ASCII       = 0x0002,  // Dump ascii values
            OFFSET      = 0x0004,  // Display address offsets
            WIDE_OFFSET = 0x0008,  // Always wide offset
            SINGLE_LINE = 0x0010,  // Hexa on one single line, no line feed, ignore other flags
            BPL         = 0x0020,  // Interpret max_line_width as number of displayed Bytes Per Line (BPL)
            C_STYLE     = 0x0040,  // C-style hexa value ("OxXX," instead of "XX")
            BINARY      = 0x0080,  // Dump binary values ("XXXXXXXX" binary digits)
            BIN_NIBBLE  = 0x0100,  // Binary values are grouped by nibble ("XXXX XXXX")
        };
    }

    // Build a string containing the hexa dump of (data, size).
    // Each line is indented by 'indent' space chars. The maximum amount
    // of chars per line is limited by line_width. If OFFSET is specified,
    // the first offset value to display is 'init_offset'.

    TSDUCKDLL std::string Hexa (const void *data,
                              size_t size,
                              uint32_t flags = hexa::HEXA,
                              size_t indent = 0,
                              size_t line_width = hexa::DEFAULT_LINE_WIDTH,
                              size_t init_offset = 0);

    // Same with a string

    TSDUCKDLL std::string Hexa (const std::string& str,
                              uint32_t flags = hexa::HEXA,
                              size_t indent = 0,
                              size_t line_width = hexa::DEFAULT_LINE_WIDTH,
                              size_t init_offset = 0);

    // Same with a ByteBlock

    TSDUCKDLL std::string Hexa (const ByteBlock& bb,
                              uint32_t flags = hexa::HEXA,
                              size_t indent = 0,
                              size_t line_width = hexa::DEFAULT_LINE_WIDTH,
                              size_t init_offset = 0);

    // Add more dump to a string.
    // Return a reference to the input string.

    TSDUCKDLL std::string& AppendHexa (std::string& str,
                                     const void *data,
                                     size_t size,
                                     uint32_t flags = hexa::HEXA,
                                     size_t indent = 0,
                                     size_t line_width = hexa::DEFAULT_LINE_WIDTH,
                                     size_t init_offset = 0);
}
