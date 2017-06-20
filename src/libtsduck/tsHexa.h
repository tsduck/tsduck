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

        //!
        //! Default line width for the ts::Hexa() family of functions.
        //!
        const size_t DEFAULT_LINE_WIDTH = 78;

        //!
        //! Flags for the ts::Hexa() family of functions.
        //!
        enum Flags {
            HEXA        = 0x0001,  //!< Dump hexa values.
            ASCII       = 0x0002,  //!< Dump ascii values.
            OFFSET      = 0x0004,  //!< Display address offsets.
            WIDE_OFFSET = 0x0008,  //!< Always wide offset.
            SINGLE_LINE = 0x0010,  //!< Hexa on one single line, no line feed, ignore other flags.
            BPL         = 0x0020,  //!< Interpret @a max_line_width as number of displayed Bytes Per Line (BPL).
            C_STYLE     = 0x0040,  //!< C-style hexa value ("0xXX," instead of "XX").
            BINARY      = 0x0080,  //!< Dump binary values ("XXXXXXXX" binary digits).
            BIN_NIBBLE  = 0x0100,  //!< Binary values are grouped by nibble ("XXXX XXXX").
        };
    }

    //!
    //! Build a multi-line string containing the hexadecimal dump of a memory area.
    //!
    //! @param [in] data Starting address of the memory area to dump.
    //! @param [in] size Size in bytes of the memory area to dump.
    //! @param [in] flags A combination of option flags indicating how to
    //! format the data. This is typically the result of or'ed values from the enum
    //! type ts::hexa::Flags.
    //! @param [in] indent Each line is indented by this number of characters.
    //! @param [in] line_width Maximum number of characters per line.
    //! If the flag @link ts::hexa::BPL @endlink is specified, @a line_width is interpreted as
    //! the number of displayed byte values per line.
    //! @param [in] init_offset If the flag @link ts::hexa::OFFSET @endlink is specified, an offset
    //! in the memory area is displayed at the beginning of each line. In this
    //! case, @a init_offset specified the offset value for the first byte.
    //! @return A string containing the formatted hexadecimal dump.
    //! Lines are separated with embedded new-line characters ('\\n').
    //! @see ts::hexa::Flags
    //!
    TSDUCKDLL std::string Hexa(const void *data,
                               size_t size,
                               uint32_t flags = hexa::HEXA,
                               size_t indent = 0,
                               size_t line_width = hexa::DEFAULT_LINE_WIDTH,
                               size_t init_offset = 0);

    //!
    //! Build a multi-line string containing the hexadecimal dump of a memory area.
    //!
    //! @param [in] str String to dump.
    //! @param [in] flags A combination of option flags indicating how to
    //! format the data. This is typically the result of or'ed values from the enum
    //! type ts::hexa::Flags.
    //! @param [in] indent Each line is indented by this number of characters.
    //! @param [in] line_width Maximum number of characters per line.
    //! If the flag @link ts::hexa::BPL @endlink is specified, @a line_width is interpreted as
    //! the number of displayed byte values per line.
    //! @param [in] init_offset If the flag @link ts::hexa::OFFSET @endlink is specified, an offset
    //! in the memory area is displayed at the beginning of each line. In this
    //! case, @a init_offset specified the offset value for the first byte.
    //! @return A string containing the formatted hexadecimal dump.
    //! Lines are separated with embedded new-line characters ('\\n').
    //! @see ts::hexa::Flags
    //!
    TSDUCKDLL std::string Hexa(const std::string& str,
                               uint32_t flags = hexa::HEXA,
                               size_t indent = 0,
                               size_t line_width = hexa::DEFAULT_LINE_WIDTH,
                               size_t init_offset = 0);

    //!
    //! Build a multi-line string containing the hexadecimal dump of a memory area.
    //!
    //! @param [in] bb Byte block to dump.
    //! @param [in] flags A combination of option flags indicating how to
    //! format the data. This is typically the result of or'ed values from the enum
    //! type ts::hexa::Flags.
    //! @param [in] indent Each line is indented by this number of characters.
    //! @param [in] line_width Maximum number of characters per line.
    //! If the flag @link ts::hexa::BPL @endlink is specified, @a line_width is interpreted as
    //! the number of displayed byte values per line.
    //! @param [in] init_offset If the flag @link ts::hexa::OFFSET @endlink is specified, an offset
    //! in the memory area is displayed at the beginning of each line. In this
    //! case, @a init_offset specified the offset value for the first byte.
    //! @return A string containing the formatted hexadecimal dump.
    //! Lines are separated with embedded new-line characters ('\\n').
    //! @see ts::hexa::Flags
    //!
    TSDUCKDLL std::string Hexa(const ByteBlock& bb,
                               uint32_t flags = hexa::HEXA,
                               size_t indent = 0,
                               size_t line_width = hexa::DEFAULT_LINE_WIDTH,
                               size_t init_offset = 0);


    //!
    //! Append a multi-line string containing the hexadecimal dump of a memory area.
    //!
    //! @param [in,out] str A string object. The formatted hexa string is appended to this object.
    //! @param [in] data Starting address of the memory area to dump.
    //! @param [in] size Size in bytes of the memory area to dump.
    //! @param [in] flags A combination of option flags indicating how to
    //! format the data. This is typically the result of or'ed values from the enum
    //! type ts::hexa::Flags.
    //! @param [in] indent Each line is indented by this number of characters.
    //! @param [in] line_width Maximum number of characters per line.
    //! If the flag @link ts::hexa::BPL @endlink is specified, @a line_width is interpreted as
    //! the number of displayed byte values per line.
    //! @param [in] init_offset If the flag @link ts::hexa::OFFSET @endlink is specified, an offset
    //! in the memory area is displayed at the beginning of each line. In this
    //! case, @a init_offset specified the offset value for the first byte.
    //! @return A reference to @a str.
    //! @see ts::hexa::Flags
    //!
    TSDUCKDLL std::string& AppendHexa(std::string& str,
                                      const void *data,
                                      size_t size,
                                      uint32_t flags = hexa::HEXA,
                                      size_t indent = 0,
                                      size_t line_width = hexa::DEFAULT_LINE_WIDTH,
                                      size_t init_offset = 0);
}
