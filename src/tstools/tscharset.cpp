//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  A utility program to encode and decode strings using various DVB and
//  and ARIB character sets.
//
//  This program is also used to generate the encoding tables for ARIB STD-B24
//  character sets. This feature is normally used only once. The generated
//  C++ source code is integrated in class ts::ARIBCharset, archived in the
//  git repository and never modified. See source file
//  src/libtsduck/dtv/charset/tsARIBCharsetEncoding.cpp
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsARIBCharset.h"
#include "tsOutputRedirector.h"
#include "tsTime.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace ts {
    class CharsetOptions: public ts::Args
    {
        TS_NOBUILD_NOCOPY(CharsetOptions);
    public:
        DuckContext duck;          // TSDuck execution context.
        bool        list;          // List all character sets names.
        bool        generate_b24;  // Generate encoding tables for ARIB STD-B24.
        bool        c_style;       // Output binary data in C/C++ syntax.
        bool        to_utf8;       // Output decoded string as UTF-8.
        bool        to_utf16;      // Output decoded string as UTF-16.
        UString     outfile;       // Output file.
        UString     encode;        // String to encode.
        ByteBlock   decode;        // Hexadecimal content to decode.

        // Print string and binary data according to formatting options.
        void printBinary(const UString& title, const ByteBlock& bin) const;
        void printString(const UString& title, const UString& str) const;

        // Constructors and destructor.
        CharsetOptions(int argc, char *argv[]);
        virtual ~CharsetOptions() override;

    private:
        // Build flags for UString::Dump().
        uint32_t dumpFlags() const;
        void printUTF8(const UString& str) const;
        void printUTF16(const UString& str) const;
    };
}


//----------------------------------------------------------------------------
//  Command line options constructors and destructor.
//----------------------------------------------------------------------------

ts::CharsetOptions::CharsetOptions(int argc, char *argv[]) :
    Args(u"Test tool for DVB and ARIB character sets", u"[options]"),
    duck(this),
    list(false),
    generate_b24(false),
    c_style(false),
    to_utf8(false),
    to_utf16(false),
    outfile(),
    encode(),
    decode()
{
    duck.defineArgsForCharset(*this);

    option(u"c-style", 'c');
    help(u"c-style",
         u"Output binary data in C/C++ syntax, using 0x prefix.");

    option(u"decode", 'd', STRING);
    help(u"decode", u"hexa-digits",
         u"Decode the specified binary data according to the default character set. "
         u"The encoded data shall be represented as binary digits. Spaces are ignored.");

    option(u"encode", 'e', STRING);
    help(u"encode", u"'string'",
         u"Encode the specified string according to the default character set.");

    option(u"from-utf-8", '8');
    help(u"from-utf-8",
         u"With --encode, specify that the parameter value is a suite of binary digits representing "
         u"the string in UTF-8 format.");

    option(u"from-utf-16", '6');
    help(u"from-utf-16",
         u"With --encode, specify that the parameter value is a suite of binary digits representing "
         u"the string in UTF-16 format. There must be an even number of bytes.");

    option(u"generate-arib-b24-encoding-table");
    help(u"generate-arib-b24-encoding-table",
         u"Generate the encoding table for ARIB STD-B24. "
         u"This is a TSDuck bootstrap tool which is used only once. "
         u"The output is C++ source code for class ts::ARIBCharset.");

    option(u"list-charsets", 'l');
    help(u"list-charsets", u"List all known character set names");

    option(u"output", 'o', FILENAME);
    help(u"output", u"Output file name. By default, use standard output.");

    option(u"to-utf-8");
    help(u"to-utf-8",
         u"With --decode (and without --verbose), display an hexadecimal representation "
         u"of the decoded string in UTF-8 format.");

    option(u"to-utf-16");
    help(u"to-utf-16",
         u"With --decode (and without --verbose), display an hexadecimal representation "
         u"of the decoded string in UTF-16 format.");

    // Analyze command line arguments.
    analyze(argc, argv);

    // Get parameter values.
    duck.loadArgs(*this);
    getValue(outfile, u"output");
    getValue(encode, u"encode");
    const UString decodeHex(value(u"decode"));
    list = present(u"list-charsets");
    generate_b24 = present(u"generate-arib-b24-encoding-table");
    c_style = present(u"c-style");
    to_utf8 = present(u"to-utf-8");
    to_utf16 = present(u"to-utf-16");
    const bool from_utf8 = present(u"from-utf-8");
    const bool from_utf16 = present(u"from-utf-16");

    // Convert input string to encode into a plain string.
    if (!encode.empty() && (from_utf8 || from_utf16)) {
        ByteBlock hex;
        if (from_utf8 && from_utf16) {
            error(u"cannot use --from-utf-8 and --from-utf-16 at the same time");
        }
        else if (!encode.hexaDecode(hex, true)) {
            error(u"invalid hexadecimal string for --encode");
        }
        else if (from_utf16 && hex.size() % 2 != 0) {
            error(u"--from-utf-16 needs an even number of bytes");
        }
        else if (from_utf8) {
            encode.assignFromUTF8(reinterpret_cast<const char*>(hex.data()), hex.size());
        }
        else { // from_utf16
            encode.resize(hex.size() / 2);
            for (size_t i = 0; i < encode.size(); ++i) {
                encode[i] = ts::UChar((uint16_t(hex[2*i]) << 8) | hex[2*i + 1]);
            }
        }
    }

    // Convert data to decode into byte block.
    if (!decodeHex.empty() && !decodeHex.hexaDecode(decode)) {
        error(u"invalid hexadecimal string for --decode");
    }

    exitOnError();
}

ts::CharsetOptions::~CharsetOptions()
{
}


//-----------------------------------------------------------------------------
// Print string and binary data according to formatting options.
//-----------------------------------------------------------------------------

// Build flags for UString::Dump().
uint32_t ts::CharsetOptions::dumpFlags() const
{
    uint32_t flags = UString::HEXA;
    if (!verbose()) {
        flags |= UString::SINGLE_LINE;
    }
    if (c_style) {
        flags |= UString::C_STYLE;
    }
    else if (verbose()) {
        flags |= UString::OFFSET;
    }
    return flags;
}

// Print encoded binary data.
void ts::CharsetOptions::printBinary(const UString& title, const ByteBlock& bin) const
{
    if (verbose()) {
        std::cout << title << " (" << bin.size() << " bytes):" << std::endl << UString::Dump(bin, dumpFlags() | UString::BPL, 2, 16);
    }
    else {
        std::cout << UString::Dump(bin, dumpFlags()) << std::endl;
    }
}

// Print string in UTF-8 hexadecimal.
void ts::CharsetOptions::printUTF8(const UString& str) const
{
    std::string u8;
    str.toUTF8(u8);
    std::cout << UString::Dump(u8.data(), u8.size(), dumpFlags() | UString::BPL, 2, 16);
    if (!verbose()) {
        std::cout << std::endl;
    }
}

// Print string in UTF-16 hexadecimal.
void ts::CharsetOptions::printUTF16(const UString& str) const
{
    const bool multi_line = verbose();

    for (size_t i = 0; i < str.length(); ++i) {
        if (!multi_line) {
            // Single line, add separator character.
            if (i > 0) {
                std::cout << " ";
            }
        }
        else if (i % 8 != 0) {
            // Multi-line, in the middle of a line.
            std::cout << " ";
        }
        else if (c_style) {
            // At start of a line, no offset.
            std::cout << "  ";
        }
        else {
            // At start of a line, with byte offset.
            std::cout << UString::Format(u"  %04X:  ", {2 * i});
        }
        std::cout << UString::Hexa(str[i], 4, UString(), c_style);
        if (c_style) {
            std::cout << (c_style ? "," : "");
        }
        if (multi_line && (i % 8 == 7 || i == str.length() - 1)) {
            std::cout << std::endl;
        }
    }
    if (!multi_line) {
        std::cout << std::endl;
    }
}

// Print plain / decoded string.
void ts::CharsetOptions::printString(const UString& title, const UString& str) const
{
    if (verbose()) {
        std::cout << title << " (" << str.size() << " characters): \"" << str << "\"" << std::endl;
        if (to_utf8) {
            printUTF8(str);
        }
        else {
            printUTF16(str);
        }
    }
    else if (to_utf8) {
        printUTF8(str);
    }
    else if (to_utf16) {
        printUTF16(str);
    }
    else {
        std::cout << str << std::endl;
    }
}


//-----------------------------------------------------------------------------
// The class ARIBCharsetCodeGenerator has access to private members
// of ARIBCharset.
//-----------------------------------------------------------------------------

namespace ts {
    class ARIBCharsetCodeGenerator
    {
        TS_NOBUILD_NOCOPY(ARIBCharsetCodeGenerator);
    public:
        // Generate the ARIB STD-24 encoding tables in the constructor.
        ARIBCharsetCodeGenerator(std::ostream& out);

    private:
        typedef ARIBCharset::CharMap CharMap;
        typedef ARIBCharset::CharRows CharRows;

        // A slice of contiguous Unicode points.
        struct Slice
        {
            bool    byte2;     // True: 2-byte mapping, false: 1-byte mapping.
            uint8_t selector;  // Selector byte (escape sequence final F).
            uint8_t row;       // Row (0x21-0x7F), always zero for 1-byte encoding.
            uint8_t index;     // Index in row (0x21-0x7F) for first character.
            uint8_t count;     // Number of characters in row (1-94).
        };

        // Map of character slices index per Unicode point base.
        typedef std::map<char32_t,Slice> SliceMap;

        // Add one slice into the slice map;
        void buildSlice(const CharMap& cmap,   // character set
                        const CharRows& rows,  // set of contiguous rows in character set
                        uint32_t row_index,    // index of row
                        uint32_t char_index,   // index of first character in row
                        uint32_t& count);      // number of character in slice, reset to zero

        // Generation steps.
        void buildTable();
        void generateFile(std::ostream& out);

        // Private members.
        SliceMap _slices;   // Map of character slices index per Unicode point base.
    };
}


//-----------------------------------------------------------------------------
// Code generator constructor.
//-----------------------------------------------------------------------------

ts::ARIBCharsetCodeGenerator::ARIBCharsetCodeGenerator(std::ostream& out) :
    _slices()
{
    buildTable();
    generateFile(out);
}


//-----------------------------------------------------------------------------
// Build the table of characters from all character sets.
//-----------------------------------------------------------------------------

void ts::ARIBCharsetCodeGenerator::buildTable()
{
    // Loop on all supported character sets.
    for (auto itmap = ARIBCharset::ALL_MAPS; *itmap != nullptr; ++itmap) {
        const ARIBCharset::CharMap& cmap(**itmap);

        // Loop on all contiguous sets of rows in the character set.
        for (uint32_t i = 0; i < ARIBCharset::MAX_ROWS; ++i) {
            const ARIBCharset::CharRows& rows(cmap.rows[i]);

            // Loop on all rows in the contiguous set of rows.
            for (uint32_t row_index = 0; row_index < rows.count; row_index++) {
                const ARIBCharset::CharRow& row(rows.rows[row_index]);

                // Locate slices of contiguous characters in the rows.
                char32_t slice_base_value = 0;
                uint32_t slice_base_index = 0;
                uint32_t slice_size = 0;

                // Loop on all characters in the row.
                for (uint32_t char_index = 0; char_index < ARIBCharset::CHAR_ROW_SIZE; ++char_index) {
                    if (row[char_index] == 0) {
                        // There is no valid Unicode point here, close previous slice if still open.
                        buildSlice(cmap, rows, row_index, slice_base_index, slice_size);
                    }
                    else {
                        // There is a valid Unicode point here.
                        if (slice_size > 0 && row[char_index] != slice_base_value + char_index - slice_base_index) {
                            // A slice exists but the current character is not contiguous.
                            // Close the current slice.
                            buildSlice(cmap, rows, row_index, slice_base_index, slice_size);
                        }
                        if (slice_size == 0) {
                            // Start a new slice.
                            slice_base_value = row[char_index];
                            slice_base_index = char_index;
                        }
                        slice_size++;
                    }
                }

                // Build last slice in this row, if still open.
                buildSlice(cmap, rows, row_index, slice_base_index, slice_size);
            }
        }
    }
}


//-----------------------------------------------------------------------------
// Build a slice of contiguous Unicode points.
//-----------------------------------------------------------------------------

void ts::ARIBCharsetCodeGenerator::buildSlice(const CharMap& cmap, const CharRows& rows, uint32_t row_index, uint32_t char_index, uint32_t& count)
{
    // Build a slice only if non empty.
    if (count > 0) {

        // Build a slice.
        Slice slice;
        slice.byte2 = cmap.byte2;
        slice.selector = cmap.selector1;
        slice.row = uint8_t(0x21 + rows.first + row_index);
        slice.index = uint8_t(0x21 + char_index);
        slice.count = uint8_t(count);

        // Reset current slice size in caller.
        count = 0;

        // Get code point of first character in slice.
        const char32_t code_point = rows.rows[row_index][char_index];
        auto it = _slices.find(code_point);
        if (it != _slices.end()) {
            // Another slice with same base exists, keep the largest one.
            if (slice.count > it->second.count) {
                it->second = slice;
            }
        }
        else {
            // No slice with same base exists, insert that one.
            _slices[code_point] = slice;

            // Get iterator on new slice.
            it = _slices.find(code_point);
            assert(it != _slices.end());

            // Check if new slice overlaps with previous one.
            if (it != _slices.begin()) {
                auto previous = it;
                --previous;
                assert(previous->first < code_point);
                if (previous->first + previous->second.count > code_point) {
                    // New slice overlaps with previous one.
                    if (previous->first + previous->second.count >= code_point + slice.count) {
                        // Current slice is fully included in previous slice, simply drop it.
                        _slices.erase(it);
                        // Must return since we did not insert anything.
                        return;
                    }
                    else {
                        // Truncate previous slice.
                        previous->second.count = uint8_t(code_point - previous->first);
                    }
                }
            }

            // Check if new slice overlaps with next one.
            auto next = it;
            ++next;
            if (next != _slices.end()) {
                assert(next->first > code_point);
                if (code_point + slice.count > next->first) {
                    // New slice overlaps with next one.
                    if (code_point + slice.count >= next->first + next->second.count) {
                        // Next slice is fully included in new slice, simply drop it.
                        _slices.erase(next);
                    }
                    else {
                        // Truncate current slice.
                        it->second.count = uint8_t(next->first - code_point);
                    }
                }
            }
        }
    }
}


//-----------------------------------------------------------------------------
// Generate the C++ source code for the encoder table.
//-----------------------------------------------------------------------------

void ts::ARIBCharsetCodeGenerator::generateFile(std::ostream& out)
{
    // Source file header.
    out << "// Automatically generated file, do not modify." << std::endl
        << "// See tscharset --generate-arib-b24-encoding-table" << std::endl
        << "// Generated " << Time::CurrentLocalTime().format(Time::DATE) << std::endl
        << std::endl
        << "#include \"tsARIBCharset.h\""<< std::endl
        << std::endl
        << "const size_t ts::ARIBCharset::ENCODING_COUNT = " << _slices.size() << ";" << std::endl
        << "const ts::ARIBCharset::EncoderEntry ts::ARIBCharset::ENCODING_TABLE[" << _slices.size() << "] = {" << std::endl;

    const uint32_t entries_per_line = 4;
    uint32_t count = 0;
    uint32_t char_total = 0;      // Total number of characters.
    uint32_t max_slice_size = 0;  // Maximum size of a slice.
    uint32_t single_slices = 0;   // Number of single-character slices.

    // Generate all slices in increasing order of base code point.
    for (const auto& it : _slices) {

        // The 32-bit encoded entry is made of 5 fields.
        const uint32_t entry =
            (it.second.byte2 ? 0x80000000 : 0x00000000) |
            (uint32_t(it.second.selector) << 24) |
            (uint32_t(it.second.row) << 16) |
            (uint32_t(it.second.index) << 8) |
            uint32_t(it.second.count);

        if (count++ % entries_per_line == 0) {
            out << "   ";
        }
        out << UString::Format(u" {0x%X, 0x%X},", {it.first, entry});
        if (count % entries_per_line == 0) {
            out << std::endl;
        }

        // Keep statistics on slice sizes.
        char_total += it.second.count;
        max_slice_size = std::max<uint32_t>(max_slice_size, it.second.count);
        if (it.second.count == 1) {
            single_slices++;
        }
    }

    // Final statistics.
    if (count % entries_per_line != 0) {
        out << std::endl;
    }
    out << "};" << std::endl
        << std::endl
        << "// Number of encodable characters: " << char_total << std::endl
        << "// Number of slices of contiguous Unicode points: " << _slices.size() << std::endl
        << "// Number of single-character slices: " << single_slices << std::endl
        << "// Maximum slice size: " << max_slice_size << std::endl;
}


//-----------------------------------------------------------------------------
// Program entry point
//-----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    ts::CharsetOptions opt(argc, argv);
    ts::OutputRedirector output(opt.outfile, opt);

    // List of character sets names.
    if (opt.list) {
        const ts::UStringList names(ts::Charset::GetAllNames());
        for (const auto& it : names) {
            std::cout << it << std::endl;
        }
    }

    // Encode a string.
    if (!opt.encode.empty()) {
        if (opt.verbose()) {
            opt.printString(u"Input", opt.encode);
        }
        opt.printBinary(u"Encoded", opt.duck.encoded(opt.encode));
    }

    // Decode a string.
    if (!opt.decode.empty()) {
        if (opt.verbose()) {
            opt.printBinary(u"Input", opt.decode);
        }
        opt.printString(u"Decoded", opt.duck.decoded(opt.decode.data(), opt.decode.size()));
    }

    // Generate the ARIB STD-B24 large encoding table from the various decoding tables.
    if (opt.generate_b24) {
        ts::ARIBCharsetCodeGenerator gen(std::cout);
    }

    return EXIT_SUCCESS;
}
