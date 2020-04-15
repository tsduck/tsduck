//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2020, Thierry Lelegard
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
//  This program is used to generate the encoding tables for ARIB STD-B24
//  character sets. See class ts::ARIBCharsetB24.
//
//  Running aribb24codegen is done only once or each time the decoding tables
//  are updated in ARIBCharsetB24. The output of aribb24codegen is C++ source
//  code which is archived in the git repository and never modified.
//
//----------------------------------------------------------------------------

#include "tsARIBCharsetB24.h"
#include "tsTime.h"
TSDUCK_SOURCE;


//-----------------------------------------------------------------------------
// The class ARIBCharsetB24CodeGenerator has access to private members
// of ARIBCharsetB24.
//-----------------------------------------------------------------------------

namespace ts {
    class ARIBCharsetB24CodeGenerator
    {
        TS_NOBUILD_NOCOPY(ARIBCharsetB24CodeGenerator);
    public:
        // Generate the ARIB STD-24 encoding tables in the constructor.
        ARIBCharsetB24CodeGenerator(std::ostream& out);

    private:
        typedef ARIBCharsetB24::CharMap CharMap;
        typedef ARIBCharsetB24::CharRows CharRows;

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

ts::ARIBCharsetB24CodeGenerator::ARIBCharsetB24CodeGenerator(std::ostream& out) :
    _slices()
{
    buildTable();
    generateFile(out);
}


//-----------------------------------------------------------------------------
// Build the table of characters from all character sets.
//-----------------------------------------------------------------------------

void ts::ARIBCharsetB24CodeGenerator::buildTable()
{
    // Loop on all supported character sets.
    for (auto itmap = ARIBCharsetB24::ALL_MAPS; *itmap != nullptr; ++itmap) {
        const ARIBCharsetB24::CharMap& cmap(**itmap);

        // Loop on all contiguous sets of rows in the character set.
        for (uint32_t i = 0; i < ARIBCharsetB24::MAX_ROWS; ++i) {
            const ARIBCharsetB24::CharRows& rows(cmap.rows[i]);

            // Loop on all rows in the contiguous set of rows.
            for (uint32_t row_index = 0; row_index < rows.count; row_index++) {
                const ARIBCharsetB24::CharRow& row(rows.rows[row_index]);

                // Locate slices of contiguous characters in the rows.
                char32_t slice_base_value = 0;
                uint32_t slice_base_index = 0;
                uint32_t slice_size = 0;

                // Loop on all characters in the row.
                for (uint32_t char_index = 0; char_index < ARIBCharsetB24::CHAR_ROW_SIZE; ++char_index) {
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

void ts::ARIBCharsetB24CodeGenerator::buildSlice(const CharMap& cmap, const CharRows& rows, uint32_t row_index, uint32_t char_index, uint32_t& count)
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

void ts::ARIBCharsetB24CodeGenerator::generateFile(std::ostream& out)
{
    // Source file header.
    out << "// Automatically generated file, do not modify." << std::endl
        << "// See internal tool aribb24codegen in src/utils." << std::endl
        << "// Generated " << Time::CurrentLocalTime().format(Time::DATE) << std::endl
        << std::endl
        << "#include \"tsARIBCharsetB24.h\""<< std::endl
        << std::endl
        << "const size_t ts::ARIBCharsetB24::ENCODING_COUNT = " << _slices.size() << ";" << std::endl
        << "const ts::ARIBCharsetB24::EncoderEntry ts::ARIBCharsetB24::ENCODING_TABLE[" << _slices.size() << "] = {" << std::endl;

    const uint32_t entries_per_line = 4;
    uint32_t count = 0;
    uint32_t char_total = 0;      // Total number of characters.
    uint32_t max_slice_size = 0;  // Maximum size of a slice.
    uint32_t single_slices = 0;   // Number of single-character slices.

    // Generate all slices in increasing order of base code point.
    for (auto it = _slices.begin(); it != _slices.end(); ++it) {

        // The 32-bit encoded entry is made of 5 fields.
        const uint32_t entry =
            (it->second.byte2 ? 0x80000000 : 0x00000000) |
            (uint32_t(it->second.selector) << 24) |
            (uint32_t(it->second.row) << 16) |
            (uint32_t(it->second.index) << 8) |
            uint32_t(it->second.count);

        if (count++ % entries_per_line == 0) {
            out << "   ";
        }
        out << UString::Format(u" {0x%X, 0x%X},", {it->first, entry});
        if (count % entries_per_line == 0) {
            out << std::endl;
        }

        // Keep statistics on slice sizes.
        char_total += it->second.count;
        max_slice_size = std::max<uint32_t>(max_slice_size, it->second.count);
        if (it->second.count == 1) {
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

int main(int argc, char* argv[])
{
    ts::ARIBCharsetB24CodeGenerator gen(std::cout);
    return EXIT_SUCCESS;
}
