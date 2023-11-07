//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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

#include "tsARIBCharset.h"
#include "tsUString.h"

// Define single instance
const ts::ARIBCharset ts::ARIBCharset::B24({u"ARIB-STD-B24", u"ARIB"});


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ARIBCharset::ARIBCharset(std::initializer_list<const UChar*> names) :
    Charset(names)
{
}


//----------------------------------------------------------------------------
// Decode a string from the specified byte buffer.
//----------------------------------------------------------------------------

bool ts::ARIBCharset::decode(UString& str, const uint8_t* data, size_t size) const
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

// The initial state for G0-G3 and GL-GR is unclear.
// There is no clear specification found in STD-B24 part 2.
// This state is based on other implementations and experimentation.
// Note: In STD-B24 part 3, chapter 8, an initialization state is described
// but it applies to captions only and is slightly different (G3 = Macro
// character set instead of Katakana).

ts::ARIBCharset::Decoder::Decoder(UString& str, const uint8_t* data, size_t size) :
    _success(true),
    _str(str),
    _data(nullptr),
    _size(0),
    _G{&KANJI_ADDITIONAL_MAP,
       &ALPHANUMERIC_MAP,
       &HIRAGANA_MAP,
       &KATAKANA_MAP},
    _GL(0),
    _GR(2),
    _lockedGL(0)
{
    decodeAll(data, size);
}


//----------------------------------------------------------------------------
// Check if next character matches c. If yes, update data and size.
//----------------------------------------------------------------------------

void ts::ARIBCharset::Decoder::decodeAll(const uint8_t* data, size_t size)
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
            // Use a "Japanese space" when GL set is not alphanumeric.
            _str.push_back(_G[_GL] == &ALPHANUMERIC_MAP ? SPACE : IDEOGRAPHIC_SPACE);
        }
        else if (match(0xA0)) {
            // Always a space in all character sets.
            // Use a "Japanese space" when GR set is not alphanumeric.
            _str.push_back(_G[_GR] == &ALPHANUMERIC_MAP ? SPACE : IDEOGRAPHIC_SPACE);
        }
        else if (*_data >= GL_FIRST && *_data <= GL_LAST) {
            // A left-side code.
            _success = decodeOneChar(_G[_GL]) && _success;
            // Restore locked shift if a single shift was used.
            _GL = _lockedGL;
        }
        else if (*_data >= GR_FIRST && *_data <= GR_LAST) {
            // A right-side code.
            _success = decodeOneChar(_G[_GR]) && _success;
        }
        else if (match(LS0)) {
            // Locking shift G0.
            _GL = _lockedGL = 0;
        }
        else if (match(LS1)) {
            // Locking shift G1.
            _GL = _lockedGL = 1;
        }
        else if (match(SS2)) {
            // Single shift G2.
            _GL = 2;
        }
        else if (match(SS3)) {
            // Single shift G3.
            _GL = 3;
        }
        else if (match(ESC)) {
            // Escape sequence.
            _success = escape() && _success;
        }
        else {
            // Character in C0 or C1 area.
            _success = processControl() && _success;
        }
    }

    // Restore previous state.
    _data = saved_data;
    _size = saved_size;
}


//----------------------------------------------------------------------------
// Check if next character matches c. If yes, update data and size.
//----------------------------------------------------------------------------

bool ts::ARIBCharset::Decoder::match(uint8_t c)
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

bool ts::ARIBCharset::Decoder::decodeOneChar(const CharMap* gset)
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
            _str.append(static_cast<uint32_t>(cp));
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

bool ts::ARIBCharset::Decoder::escape()
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
    // Execute the command from the sequence.
    switch (seq) {
        case 0x00000000: {
            // No intermediate sequence, just ESC F, assign GL or GR.
            switch (F) {
                case 0x6E:
                    // LS2: Locking shift G2.
                    _GL = _lockedGL = 2;
                    return true;
                case 0x6F:
                    // LS3: Locking shift G3.
                    _GL = _lockedGL = 3;
                    return true;
                case 0x7E:
                    // LS1R: Locking shift G1R.
                    _GR = 1;
                    return true;
                case 0x7D:
                    // LS2R: Locking shift G2R.
                    _GR = 2;
                    return true;
                case 0x7C:
                    // LS3R: Locking shift G3R.
                    _GR = 3;
                    return true;
                default:
                    // Unsupported function.
                    return false;
            }
        }
        case 0x00000028:   // 1-byte G set -> G0
        case 0x00000024: { // 2-byte G set -> G0
            _G[0] = finalToCharMap(F, true);
            return true;
        }
        case 0x00000029:   // 1-byte G set -> G1
        case 0x00002429: { // 2-byte G set -> G1
            _G[1] = finalToCharMap(F, true);
            return true;
        }
        case 0x0000002A:   // 1-byte G set -> G2
        case 0x0000242A: { // 2-byte G set -> G2
            _G[2] = finalToCharMap(F, true);
            return true;
        }
        case 0x0000002B:   // 1-byte G set -> G3
        case 0x0000242B: { // 2-byte G set -> G3
            _G[3] = finalToCharMap(F, true);
            return true;
        }
        case 0x00002820:   // 1-byte DRCS -> G0
        case 0x00242820: { // 2-byte DRCS -> G0
            _G[0] = finalToCharMap(F, false);
            return true;
        }
        case 0x00002920:   // 1-byte DRCS -> G1
        case 0x00242920: { // 2-byte DRCS -> G1
            _G[1] = finalToCharMap(F, false);
            return true;
        }
        case 0x00002A20:   // 1-byte DRCS -> G2
        case 0x00242A20: { // 2-byte DRCS -> G2
            _G[2] = finalToCharMap(F, false);
            return true;
        }
        case 0x00002B20:   // 1-byte DRCS -> G3
        case 0x00242B20: { // 2-byte DRCS -> G3
            _G[3] = finalToCharMap(F, false);
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

const ts::ARIBCharset::CharMap* ts::ARIBCharset::Decoder::finalToCharMap(uint8_t f, bool gset_not_drcs) const
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
// Process a character in C0 or C1 areas.
//----------------------------------------------------------------------------

bool ts::ARIBCharset::Decoder::processControl()
{
    // We currently support none of these sequences.
    // But we need to properly skip the right number of bytes in the sequence.
    size_t len = 0;

    switch (*_data) {
        case PAPF:
        case COL:
        case POL:
        case SZX:
        case FLC:
        case WMM:
        case RPC:
        case HLC:
            // 2-byte sequences (P1 parameter)
            len = 2;
            break;
        case APS:
        case TIME:
            // 3-byte sequences (P1, P2 parameters)
            len = 3;
            break;
        case CDC:
            // Variable length.
            len = _size >= 2 && _data[1] == 0x20 ? 3 : 2;
            break;
        case MACRO:
            // Variable length, end with MACRO 0x4F.
            for (len = 1; len < _size && (_data[len-1] != MACRO || _data[len] != 0x4F); ++len) {
            }
            ++len;
            break;
        case CSI:
            // Variable length, end with code > 0x40.
            for (len = 1; len < _size && _data[len] < 0x40; ++len) {
            }
            ++len;
            break;
        default:
            // By default, other characters are 1-byte commands.
            len = 1;
            break;
    }

    // Skip the sequence.
    len = std::min(len, _size);
    _data += len;
    _size -= len;

    // All sequences are unsupported.
    return false;
}


//----------------------------------------------------------------------------
// Find the encoding entry for a Unicode point.
//----------------------------------------------------------------------------

size_t ts::ARIBCharset::FindEncoderEntry(char32_t code_point, size_t hint)
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

bool ts::ARIBCharset::canEncode(const UString& str, size_t start, size_t count) const
{
    const size_t len = str.length();
    const size_t end = count > len ? len : std::min(len, start + count);

    // Look for an encoding entry for each character.
    size_t index = 0;
    for (size_t i = start; i < end; ++i) {
        const UChar c = str[i];

        // Space is not in the encoding table but is always valid.
        if (c != SPACE && c != IDEOGRAPHIC_SPACE) {
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
// Encode a C++ Unicode string into an ARIB string.
//----------------------------------------------------------------------------

size_t ts::ARIBCharset::encode(uint8_t*& buffer, size_t& size, const UString& str, size_t start, size_t count) const
{
    const size_t len = str.length();
    if (buffer == nullptr || size == 0 || start >= len) {
        return 0;
    }
    else {
        const UChar* const initial = str.data() + start;
        const UChar* in = initial;
        size_t in_count = count >= len || start + count >= len ? (len - start) : count;
        Encoder enc(buffer, size, in, in_count);
        return in - initial;
    }
}


//----------------------------------------------------------------------------
// An internal encoder class.
//----------------------------------------------------------------------------

ts::ARIBCharset::Encoder::Encoder(uint8_t*& out, size_t& out_size, const UChar*& in, size_t& in_count) :
    _G{KANJI_ADDITIONAL_MAP.selector1,   // Same initial state as decoding engine
       ALPHANUMERIC_MAP.selector1,
       HIRAGANA_MAP.selector1,
       KATAKANA_MAP.selector1},
    _byte2{KANJI_ADDITIONAL_MAP.byte2,   // Same order as _G
           ALPHANUMERIC_MAP.byte2,
           HIRAGANA_MAP.byte2,
           KATAKANA_MAP.byte2},
    _GL(0),             // G0 -> GL
    _GR(2),             // G2 -> GR
    _GL_last(false),    // First charset switch will use GL
    _Gn_history(0x3210) // G3=oldest, G0=last-used
{
    // Previous index in encoding table.
    size_t prev_index = NPOS;

    // Loop on input UTF-16 characters.
    while (in_count > 0 && out_size > 0) {

        // Get unicode code point (1 or 2 UChar from input).
        char32_t cp = *in;
        size_t cp_size = 1;
        if (IsLeadingSurrogate(*in)) {
            // Need a second UChar.
            if (in_count < 2) {
                // End of string, truncated surrogate pair. Consume the first char so that
                // the caller does not infinitely try to decode the rest of the string.
                ++in;
                in_count = 0;
                return;
            }
            else {
                // Rebuild the Unicode point from the pair.
                cp = FromSurrogatePair(in[0], in[1]);
                cp_size = 2;
            }
        }

        // Find the entry for this code point in the encoding table.
        const size_t index = FindEncoderEntry(cp, prev_index);
        if (index != NPOS) {
            // This character is encodable.
            assert(index < ENCODING_COUNT);
            const EncoderEntry& enc(ENCODING_TABLE[index]);
            prev_index = index;

            // Make sure the right character set is selected.
            // Insert the corresponding escape sequence if necessary.
            // Also make sure that the encoded sequence will fit in output buffer.
            if (!selectCharSet(out, out_size, enc.selectorF(), enc.byte2())) {
                // Cannot insert the right sequence. Do not attempt to encode the code point.
                return;
            }

            // Insert the encoded code point (1 or 2 bytes).
            assert(cp >= enc.code_point);
            assert(cp < enc.code_point + enc.count());
            assert(cp - enc.code_point + enc.index() <= GL_LAST);
            const uint8_t mask = enc.selectorF() == _G[_GR] ? 0x80 : 0x00;
            if (enc.byte2()) {
                // 2-byte character set, insert row first.
                assert(out_size >= 2);
                *out++ = enc.row() | mask;
                --out_size;
            }
            assert(out_size >= 1);
            *out++ = uint8_t(cp - enc.code_point + enc.index()) | mask;
            --out_size;
        }
        else if ((cp == SPACE || cp == IDEOGRAPHIC_SPACE) && !encodeSpace(out, out_size, cp == IDEOGRAPHIC_SPACE)) {
            // Tried to encode a space but failed.
            return;
        }

        // Now, the character has been successfully encoded (or ignored if not encodable).
        // Remove it from the input buffer.
        in += cp_size;
        in_count -= cp_size;
    }
}


//----------------------------------------------------------------------------
// Check if Gn (n=0-3) is alphanumeric.
//----------------------------------------------------------------------------

bool ts::ARIBCharset::Encoder::isAlphaNumeric(uint8_t index) const
{
    return _G[index] == ALPHANUMERIC_MAP.selector1 || _G[index] == ALPHANUMERIC_MAP.selector2;
}


//----------------------------------------------------------------------------
// Encode a space, alphanumeric or ideographic.
//----------------------------------------------------------------------------

bool ts::ARIBCharset::Encoder::encodeSpace(uint8_t*& out, size_t& out_size, bool ideographic)
{
    uint8_t code = 0;
    size_t count = 0;

    if (ideographic) {
        // Insert a space SP (0x20) in any ideographic (non-alphanumeric) character set.
        // We assume that at least GL or GR is not alphanumeric, so there is not need to switch character set.
        // If the two are ideographic, try to find one which uses 1-byte encoding.
        if (!_byte2[_GL] && !isAlphaNumeric(_GL)) {
            code = SP;
            count = 1;
        }
        else if (!_byte2[_GR] && !isAlphaNumeric(_GR)) {
            code = SP | 0x80;
            count = 1;
        }
        else if (!isAlphaNumeric(_GL)) {
            assert(_byte2[_GL]);
            code = SP;
            count = 2;
        }
        else {
            assert(_byte2[_GR] && !isAlphaNumeric(_GR));
            code = SP | 0x80;
            count = 2;
        }
    }
    else {
        // Insert a space SP (0x20) in alphanumeric character set.
        if (isAlphaNumeric(_GL)) {
            code = SP;
            count = 1;
        }
        else if (isAlphaNumeric(_GR)) {
            code = SP | 0x80;
            count = 1;
        }
        else if (selectCharSet(out, out_size, ALPHANUMERIC_MAP.selector1, false)) {
            code = ALPHANUMERIC_MAP.selector1 == _G[_GR] ? (SP | 0x80) : SP;
            count = 1;
        }
        else {
            // Cannot insert the sequence to switch to alphanumeric character set.
            return false;
        }
    }

    // Insert the encoded space.
    if (count > out_size) {
        return false;
    }
    else {
        while (count-- > 0) {
            *out++ = code;
            --out_size;
        }
        return true;
    }
}


//----------------------------------------------------------------------------
// Switch to a given character set (from selector F).
// Not really optimized. We always keep the same character set in G0.
//----------------------------------------------------------------------------

bool ts::ARIBCharset::Encoder::selectCharSet(uint8_t*& out, size_t& out_size, uint8_t selectorF, bool byte2)
{
    // Required space for one character after escape sequence.
    const size_t char_size = byte2 ? 2 : 1;

    // An escape sequence is up to 7 bytes.
    uint8_t seq[7];
    size_t seq_size = 0;

    // There is some switching sequence to add only if the charset is neither in GL nor GR.
    if (selectorF != _G[_GL] && selectorF != _G[_GR]) {
        // If the charset is not in G0-G3, we need to load it in one of them.
        if (selectorF != _G[0] && selectorF != _G[1] && selectorF != _G[2] && selectorF != _G[3]) {
            seq_size = selectG0123(seq, selectorF, byte2);
        }
        // Route the right Gx in either GL or GR.
        seq_size += selectGLR(seq + seq_size, selectorF);
    }

    // Finally, insert the escape sequence if there is enough room for it plus one character.
    if (seq_size + char_size > out_size) {
        return false;
    }
    if (seq_size > 0) {
        assert(seq_size < sizeof(seq));
        std::memcpy(out, seq, seq_size);
        out += seq_size;
        out_size -= seq_size;
    }

    // Keep track of last GL/GR used.
    _GL_last = _G[_GL] == selectorF;
    return true;
}


//----------------------------------------------------------------------------
// Select GL/GR from G0-3 for a given selector F. Return escape sequence size.
//----------------------------------------------------------------------------

size_t ts::ARIBCharset::Encoder::selectGLR(uint8_t* seq, uint8_t F)
{
    // If GL was last used, use GR and vice versa.
    if (F == _G[0]) {
        // G0 can be routed to GL only.
        _GL = 0;
        seq[0] = LS0;
        return 1;
    }
    else if (F == _G[1]) {
        if (_GL_last) {
            _GR = 1;
            seq[0] = ESC; seq[1] = 0x7E;
            return 2;

        }
        else {
            _GL = 1;
            seq[0] = LS1;
            return 1;
        }
    }
    else if (F == _G[2]) {
        if (_GL_last) {
            _GR = 2;
            seq[0] = ESC; seq[1] = 0x7D;
            return 2;

        }
        else {
            _GL = 2;
            seq[0] = ESC; seq[1] = 0x6E;
            return 2;
        }
    }
    else {
        assert(F == _G[3]);
        if (_GL_last) {
            _GR = 3;
            seq[0] = ESC; seq[1] = 0x7C;
            return 2;
        }
        else {
            _GL = 3;
            seq[0] = ESC; seq[1] = 0x6F;
            return 2;
        }
    }
}


//----------------------------------------------------------------------------
// Set G0-3 to a given selector F. Return escape sequence size.
//----------------------------------------------------------------------------

size_t ts::ARIBCharset::Encoder::selectG0123(uint8_t* seq, uint8_t F, bool byte2)
{
    // Get index oldest used charset. Reuse it. Mark it as last used.
    const uint8_t index = uint8_t(_Gn_history >> 12) & 0x03;
    _Gn_history = uint16_t(_Gn_history << 4) | index;

    // Assign the new character set.
    _G[index] = F;
    _byte2[index] = byte2;

    // Generate the escape sequence.
    // ARIB STD-B24, part 2, chapter 7, table 7-2
    if (!byte2) {
        // 1-byte G set in G0-3
        seq[0] = ESC;
        seq[1] = 0x28 + index;
        seq[2] = F;
        return 3;
    }
    else if (index == 0) {
        // 2-byte G set in G0
        seq[0] = ESC;
        seq[1] = 0x24;
        seq[2] = F;
        return 3;
    }
    else {
        // 2-byte G set in G1-3
        seq[0] = ESC;
        seq[1] = 0x24;
        seq[2] = 0x28 + index;
        seq[3] = F;
        return 4;
    }
}
