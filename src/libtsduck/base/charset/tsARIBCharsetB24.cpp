//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//
// Invocation of code elements
// ---------------------------
// ARIB STD-B24, part 2, chapter 7, table 7-1
//
//            Codes           Code  => Invocation  Invocation
//   Acronym  representation  element  area        effect
//
//   LS0      0F              G0       GL          Locking shift
//   LS1      0E              G1       GL          Locking shift
//   LS2      1B 6E           G2       GL          Locking shift
//   LS3      1B 6F           G3       GL          Locking shift
//   LS1R     1B 7E           G1       GR          Locking shift
//   LS2R     1B 7D           G2       GR          Locking shift
//   LS3R     1B 7C           G3       GR          Locking shift
//   SS2      19              G2       GL          Single shift
//   SS3      1D              G3       GL          Single shift
//
//
// Designation of graphic sets
// ---------------------------
// ARIB STD-B24, part 2, chapter 7, table 7-2
//
//   Codes           Classification   Designated
//   representation  of graphic sets  element
//
//   1B 28 F         1-byte G set     G0
//   1B 29 F         -                G1
//   1B 2A F         -                G2
//   1B 2B F         -                G3
//   1B 24 F         2-byte G set     G0
//   1B 24 29 F      -                G1
//   1B 24 2A F      -                G2
//   1B 24 2B F      -                G3
//   1B 28 20 F      1-byte DRCS      G0
//   1B 29 20 F      -                G1
//   1B 2A 20 F      -                G2
//   1B 2B 20 F      -                G3
//   1B 24 28 20 F   2-byte DRCS      G0
//   1B 24 29 20 F   -                G1
//   1B 24 2A 20 F   -                G2
//   1B 24 2B 20 F   -                G3
//
//
// Classification of code set and final byte (F)
// ---------------------------------------------
// ARIB STD-B24, part 2, chapter 7, table 7-3
//
//   Classification                             Final
//   of graphic sets  Graphic sets              (F)    Remarks
//
//   G set            Kanji                     42     2-byte code
//   -                Alphanumeric              4A     1-byte code
//   -                Hiragana                  30     1-byte code
//   -                Katakana                  31     1-byte code
//   -                Mosaic A                  32     1-byte code
//   -                Mosaic B                  33     1-byte code
//   -                Mosaic C                  34     1-byte code, non-spacing
//   -                Mosaic D                  35     1-byte code, non-spacing
//   -                Proportional alphanumeric 36     1-byte code
//   -                Proportional hiragana     37     1-byte code
//   -                Proportional katakana     38     1-byte code
//   -                JIS X 0201 katakana       49     1-byte code
//   -                JIS comp. Kanji Plane 1   39     2-byte code
//   -                JIS comp. Kanji Plane 2   3A     2-byte code
//   -                Additional symbols        3B     2-byte code
//   DRCS             DRCS-0                    40     2-byte code
//   -                DRCS-1                    41     1-byte code
//   -                DRCS-2                    42     1-byte code
//   -                DRCS-3                    43     1-byte code
//   -                DRCS-4                    44     1-byte code
//   -                DRCS-5                    45     1-byte code
//   -                DRCS-6                    46     1-byte code
//   -                DRCS-7                    47     1-byte code
//   -                DRCS-8                    48     1-byte code
//   -                DRCS-9                    49     1-byte code
//   -                DRCS-10                   4A     1-byte code
//   -                DRCS-11                   4B     1-byte code
//   -                DRCS-12                   4C     1-byte code
//   -                DRCS-13                   4D     1-byte code
//   -                DRCS-14                   4E     1-byte code
//   -                DRCS-15                   4F     1-byte code
//   -                Macro                     70     1-byte code
//
//
//----------------------------------------------------------------------------

#include "tsARIBCharsetB24.h"
#include "tsUString.h"
#include "tsByteBlock.h"
TSDUCK_SOURCE;

// Define singleton instance
TS_DEFINE_SINGLETON(ts::ARIBCharsetB24);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ARIBCharsetB24::ARIBCharsetB24() :
    Charset(u"ARIB-STD-B24")
{
}


//----------------------------------------------------------------------------
// Decode a DVB string from the specified byte buffer.
//----------------------------------------------------------------------------

bool ts::ARIBCharsetB24::decode(UString& str, const uint8_t* data, size_t size) const
{
    // Try to minimize reallocation.
    str.clear();
    str.reserve(size);

    // Perform decoding.
    Decoder dec(str, data, size);
    return dec.success();
}


//----------------------------------------------------------------------------
// An internal decoder class. Using ARIB STD-B24 notation.
//----------------------------------------------------------------------------

ts::ARIBCharsetB24::Decoder::Decoder(UString& str, const uint8_t* data, size_t size) :
    _success(true),
    _str(str),
    _data(nullptr),
    _size(0),
    _G0(&KANJI_ADDITIONAL_MAP),  // The initial state for G0-G3 and GL-GR is unclear.
    _G1(&ALPHANUMERIC_MAP),      // No clear specification found in STD-B24.
    _G2(&HIRAGANA_MAP),          // This state is based on other implementations and experimentation.
    _G3(&KATAKANA_MAP),
    _GL(_G0),
    _GR(_G2),
    _lockedGL(_GL)
{
    decodeAll(data, size);
}


//----------------------------------------------------------------------------
// Check if next character matches c. If yes, update data and size.
//----------------------------------------------------------------------------

void ts::ARIBCharsetB24::Decoder::decodeAll(const uint8_t* data, size_t size)
{
    // Filter out invalid parameters.
    if (data == nullptr) {
        _success = false;
        return;
    }

    // Save previous state, setup new state.
    const uint8_t* const saved_data = _data;
    const size_t saved_size = _size;
    _data = data;
    _size = size;

    // Loop in input byte sequences.
    while (_size > 0) {
        if (match(0x20)) {
            // Always a space in all character sets.
            _str.push_back(u' ');
        }
        else if (*_data >= GL_FIRST && *_data <= GL_LAST) {
            // A left-side code.
            _success = decodeOneChar(_GL) && _success;
            // Restore locked shift if a single shift was used.
            _GL = _lockedGL;
        }
        else if (*_data >= GR_FIRST && *_data <= GR_LAST) {
            // A right-side code.
            _success = decodeOneChar(_GR) && _success;
        }
        else if (match(LS0)) {
            // Locking shift G0.
            _GL = _lockedGL = _G0;
        }
        else if (match(LS1)) {
            // Locking shift G1.
            _GL = _lockedGL = _G1;
        }
        else if (match(SS2)) {
            // Single shift G2.
            _GL = _G2;
        }
        else if (match(SS3)) {
            // Single shift G3.
            _GL = _G3;
        }
        else if (match(ESC)) {
            // Escape sequence.
            _success = escape() && _success;
        }
        else {
            // Unsupported character.
            _success = false;
        }
    }

    // Restore previous state.
    _data = saved_data;
    _size = saved_size;
}


//----------------------------------------------------------------------------
// Check if next character matches c. If yes, update data and size.
//----------------------------------------------------------------------------

bool ts::ARIBCharsetB24::Decoder::match(uint8_t c)
{
    if (_size > 0 && *_data == c) {
        _data++;
        _size--;
        return true;
    }
    else {
        return false;
    }
}


//----------------------------------------------------------------------------
// Decode one character and append to str.
//----------------------------------------------------------------------------

bool ts::ARIBCharsetB24::Decoder::decodeOneChar(const CharMap* gset)
{
    // Filter truncated data.
    if (gset == nullptr || _size == 0) {
        return false;
    }

    // Get first and optional second byte, transform them as index in slice.
    size_t b1 = GL_FIRST;
    if (gset->byte2) {
        // Need a row index in first byte.
        b1 = (*_data++ & 0x7F);
        _size--;
        if (_size == 0) {
            return false; // truncated data
        }
    }
    // Get second byte, index in the row.
    size_t b2 = (*_data++ & 0x7F);
    _size--;
    // Check byte values.
    if (b1 < GL_FIRST || b1 > GL_LAST || b2 < GL_FIRST || b2 > GL_LAST) {
        return false; // out of range
    }
    b1 -= GL_FIRST;
    b2 -= GL_FIRST;

    // Now interpret the [b1]-b2 bytes.
    if (gset->macro) {
        // This is the macro character set.
        // Currently, we only support the predefined macros.
        if (b1 == 0 && b2 >= PREDEF_MACRO_BASE && b2 < PREDEF_MACRO_BASE + PREDEF_MACRO_COUNT) {
            // This is a predefined macro.
            const size_t index = b2 - PREDEF_MACRO_BASE;
            decodeAll(PREDEF_MACROS[index].content, PREDEF_MACROS[index].size);
            return true;
        }
        else {
            // This is an unknown macro.
            return false;
        }
    }
    else {
        // This is a table-based character set.
        // Get the 32-bit code point from the map.
        char32_t cp = 0;
        for (size_t i = 0; i < MAX_ROWS; ++i) {
            const CharRows& rows(gset->rows[i]);
            if (rows.count == 0) {
                // End of map.
                break;
            }
            if (b1 >= rows.first && b1 < rows.first + rows.count && rows.rows != nullptr) {
                // The character is in this row.
                cp = rows.rows[b1 - rows.first][b2];
                break;
            }
        }

        // Insert code point, if one was found.
        if (cp != 0) {
            _str.append(cp);
            return true;
        }
        else {
            return false;
        }
    }
}


//----------------------------------------------------------------------------
// Process an escape sequence starting at current byte.
//----------------------------------------------------------------------------

bool ts::ARIBCharsetB24::Decoder::escape()
{
    // The ESC character is already consumed.
    // Get all intermediate sequence characters, in range 0x20-0x2F, and final byte F.
    // The intermediate characters are read as a 32-bit value (max: 4-byte sequence).
    // Example sequence: 1B 24 2B 20 42 -> seq = 0x00242B20, F = 0x42.

    // Get intermediate sequence.
    uint32_t seq = 0;
    while (_size > 0 && *_data >= 0x20 && *_data <= 0x2F) {
        seq <<= 8;
        seq |= *_data++;
        _size--;
    }

    // Get final byte F.
    if (_size == 0) {
        return false; // truncated escape sequence
    }
    const uint8_t F = *_data++;
    _size--;

    // Now the escape sequence has been properly removed from the string.

    switch (seq) {
        case 0x00000000: {
            // No intermediate sequence, just ESC F, assign GL or GR.
            switch (F) {
                case 0x6E:
                    // LS2: Locking shift G2.
                    _GL = _lockedGL = _G2;
                    return true;
                case 0x6F:
                    // LS3: Locking shift G3.
                    _GL = _lockedGL = _G3;
                    return true;
                case 0x7E:
                    // LS1R: Locking shift G1R.
                    _GR = _G1;
                    return true;
                case 0x7D:
                    // LS2R: Locking shift G2R.
                    _GR = _G2;
                    return true;
                case 0x7C:
                    // LS3R: Locking shift G3R.
                    _GR = _G3;
                    return true;
                default:
                    // Unsupported function.
                    return false;
            }
        }
        case 0x00000028:   // 1-byte G set -> G0
        case 0x00000024: { // 2-byte G set -> G0
            _G0 = finalToCharMap(F, true);
            return true;
        }
        case 0x00000029:   // 1-byte G set -> G1
        case 0x00002429: { // 2-byte G set -> G1
            _G1 = finalToCharMap(F, true);
            return true;
        }
        case 0x0000002A:   // 1-byte G set -> G2
        case 0x0000242A: { // 2-byte G set -> G2
            _G2 = finalToCharMap(F, true);
            return true;
        }
        case 0x0000002B:   // 1-byte G set -> G3
        case 0x0000242B: { // 2-byte G set -> G3
            _G3 = finalToCharMap(F, true);
            return true;
        }
        case 0x00002820:   // 1-byte DRCS -> G0
        case 0x00242820: { // 2-byte DRCS -> G0
            _G0 = finalToCharMap(F, false);
            return true;
        }
        case 0x00002920:   // 1-byte DRCS -> G1
        case 0x00242920: { // 2-byte DRCS -> G1
            _G1 = finalToCharMap(F, false);
            return true;
        }
        case 0x00002A20:   // 1-byte DRCS -> G2
        case 0x00242A20: { // 2-byte DRCS -> G2
            _G2 = finalToCharMap(F, false);
            return true;
        }
        case 0x00002B20:   // 1-byte DRCS -> G3
        case 0x00242B20: { // 2-byte DRCS -> G3
            _G3 = finalToCharMap(F, false);
            return true;
        }
        default: {
            // Unsupported escape sequence
            return false;
        }
    }
}


//----------------------------------------------------------------------------
// Get a character set from an ESC sequence "final byte" F.
//----------------------------------------------------------------------------

const ts::ARIBCharsetB24::CharMap* ts::ARIBCharsetB24::Decoder::finalToCharMap(uint8_t f, bool gset_not_drcs) const
{
    if (f == 0) {
        // Invalid value, used as marker in tables, so filter it now.
        return &UNSUPPORTED_1BYTE;
    }
    else if (gset_not_drcs) {
        // Look for known character sets in the list of tables.
        for (auto it = ALL_MAPS; *it != nullptr; ++it) {
            const CharMap& cm(**it);
            if (f == cm.selector1 || f == cm.selector2) {
                return *it;
            }
        }
        // Not found, either a Mosaic 1-byte code or an unvalid F value.
        return &UNSUPPORTED_1BYTE;
    }
    else if (f == 0x70) {
        // Macro 1-byte code (not yet supported here)
        return &UNSUPPORTED_1BYTE;
    }
    else if (f == 0x40) {
        // DRCS-0 2-byte code
        return &UNSUPPORTED_2BYTE;
    }
    else {
        // DRCS-1 to DRCS-15 1-byte code or an unvalid F value.
        return &UNSUPPORTED_1BYTE;
    }
}


//----------------------------------------------------------------------------
// Find the encoding entry for a Unicode point.
//----------------------------------------------------------------------------

size_t ts::ARIBCharsetB24::FindEncoderEntry(char32_t code_point, size_t hint)
{
    // If a hint is specified, tried this slice, its next and its previous.
    if (hint < ENCODING_COUNT) {
        if (ENCODING_TABLE[hint].contains(code_point)) {
            // Found in same slice.
            return hint;
        }
        else if (hint + 1 < ENCODING_COUNT && ENCODING_TABLE[hint + 1].contains(code_point)) {
            // Found in next slice.
            return hint + 1;
        }
        else if (hint > 0 && ENCODING_TABLE[hint - 1].contains(code_point)) {
            // Found in previous slice.
            return hint - 1;
        }
        // Code point is too far, hint was useless, try standard method.
    }

    // Dichotomic search.
    size_t begin = 0;
    size_t end = ENCODING_COUNT;

    while (begin < end) {
        const size_t mid = begin + (end - begin) / 2;
        if (ENCODING_TABLE[mid].contains(code_point)) {
            return mid;
        }
        else if (code_point < ENCODING_TABLE[mid].code_point) {
            end = mid;
        }
        else {
            begin = mid + 1;
        }
    }

    return NPOS;
}


//----------------------------------------------------------------------------
// Check if a string can be encoded using the charset.
//----------------------------------------------------------------------------

bool ts::ARIBCharsetB24::canEncode(const UString& str, size_t start, size_t count) const
{
    const size_t len = str.length();
    const size_t end = count > len ? len : std::min(len, start + count);

    // Look for an encoding entry for each character.
    size_t index = 0;
    for (size_t i = start; i < end; ++i) {
        const UChar c = str[i];

        // Space is not in the encoding table but is always valid.
        if (c != u' ') {
            if (!IsLeadingSurrogate(c)) {
                // 16-bit code point
                index = FindEncoderEntry(c, index);
            }
            else if (++i >= len) {
                // Truncated surrogate pair.
                return false;
            }
            else {
                // Rebuilt 32-bit code point from surrogate pair.
                index = FindEncoderEntry(FromSurrogatePair(c, str[i]), index);
            }
            // Stop when a character cannot be encoded.
            if (index == NPOS) {
                return false;
            }
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Encode a C++ Unicode string into a DVB string.
//----------------------------------------------------------------------------

size_t ts::ARIBCharsetB24::encode(uint8_t*& buffer, size_t& size, const UString& str, size_t start, size_t count) const
{
    //@@@@@
    return 0;
}
