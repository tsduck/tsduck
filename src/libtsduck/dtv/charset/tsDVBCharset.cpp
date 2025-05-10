//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBCharset.h"
#include "tsUString.h"
#include "tsDVBCharTableSingleByte.h"
#include "tsDVBCharTableUTF8.h"

// Default predefined DVB character set (using ISO-6937 as default table).
// The charset table registers itself during initialization.
const ts::DVBCharset ts::DVBCharset::DVB({u"ISO-6937", u"DVB"}, DVBCharTableSingleByte::RAW_ISO_6937);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::DVBCharset::DVBCharset(const UChar* name, const DVBCharTable& default_table) :
    Charset(name),
    _default_table(default_table)
{
}

ts::DVBCharset::DVBCharset(std::initializer_list<const UChar*> names, const DVBCharTable& default_table) :
    Charset(names),
    _default_table(default_table)
{
}


//----------------------------------------------------------------------------
// Check if a string can be encoded using the charset.
//----------------------------------------------------------------------------

bool ts::DVBCharset::canEncode(const UString& str, size_t start, size_t count) const
{
    // Everything is encodable using DVB character set because UTF-8 and UTF-16
    // are part of the DVB character tables and they can encode everything.
    return true;
}


//----------------------------------------------------------------------------
// Decode a DVB string from the specified byte buffer.
//----------------------------------------------------------------------------

bool ts::DVBCharset::decode(UString& str, const uint8_t* data, size_t size) const
{
    // Try to minimize reallocation.
    str.clear();
    str.reserve(size);

    // Null or empty buffer is a valid empty string.
    if (data == nullptr || size == 0) {
        return true;
    }

    // Get the DVB character set code from the beginning of the string.
    uint32_t code = 0;
    size_t codeSize = 0;
    if (!DVBCharTable::DecodeTableCode(code, codeSize, data, size)) {
        return false;
    }

    // Skip the character code.
    assert(codeSize <= size);
    data += codeSize;
    size -= codeSize;

    // Get the character set for this DVB string.
    const DVBCharTable* table = code == 0 ? &_default_table : DVBCharTable::GetTableFromLeadingCode(code);
    if (table == nullptr) {
        // Unsupported character table. Collect all ANSI characters, replace others by '.'.
        for (size_t i = 0; i < size; i++) {
            str.push_back(data[i] >= 0x20 && data[i] <= 0x7E ? UChar(data[i]) : FULL_STOP);
        }
        return false;
    }
    else {
        // Convert the DVB string using the character table.
        table->decode(str, data, size);
        return true;
    }
}


//----------------------------------------------------------------------------
// Encode a C++ Unicode string into a DVB string.
//----------------------------------------------------------------------------

size_t ts::DVBCharset::encode(uint8_t*& buffer, size_t& size, const UString& str, size_t start, size_t count) const
{
    // Sanitize start and count.
    const size_t length = str.length();
    start = std::min(start, length);
    count = std::min(count, length - start);

    // Skip cases where there is nothing to do.
    if (buffer == nullptr || size == 0 || count == 0) {
        return 0;
    }

    // Look for a character set which can encode the string.
    // First, try using default table for this charset.
    const DVBCharTable* table = _default_table.canEncode(str, start, count) ? &_default_table : nullptr;

    // Then, try to encode using these character tables in order.
    const auto& lookup_tables(GetPreferredCharsets());
    for (size_t i = 0; table == nullptr && i < lookup_tables.size(); ++i) {
        // Skip default table since it has been tried first.
        if (lookup_tables[i] != &_default_table && lookup_tables[i]->canEncode(str, start, count)) {
            table = lookup_tables[i];
        }
    }
    if (table == nullptr) {
        // Should not happen since UTF-8 can encode everything.
        return 0;
    }

    // Serialize the table code.
    table->encodeTableCode(buffer, size);

    // Encode the string.
    return table->encode(buffer, size, str, start, count);
}


//----------------------------------------------------------------------------
// Get an ordered list of character sets which are used to encode DVB strings.
//----------------------------------------------------------------------------

const std::vector<const ts::DVBCharTable*>& ts::DVBCharset::GetPreferredCharsets()
{
    // Thread-safe init-safe static data pattern:
    static const std::vector<const ts::DVBCharTable*> charsets {
        // No leading character table code.
        &DVBCharTableSingleByte::RAW_ISO_6937,     // Default DVB table, ISO-6937 with addition of the Euro symbol
        // 1-byte leading character table code.
        &DVBCharTableSingleByte::RAW_ISO_8859_15,  // Latin-9, Latin/Western European alphabet
        &DVBCharTableSingleByte::RAW_ISO_8859_10,  // Latin-6, Latin/Nordic alphabet
        &DVBCharTableSingleByte::RAW_ISO_8859_13,  // Latin-7, Latin/Baltic Rim alphabet
        &DVBCharTableSingleByte::RAW_ISO_8859_14,  // Latin-8, Latin/Celtic alphabet
        &DVBCharTableSingleByte::RAW_ISO_8859_5,   // Latin/Cyrillic alphabet
        &DVBCharTableSingleByte::RAW_ISO_8859_7,   // Latin/Greek alphabet
        &DVBCharTableSingleByte::RAW_ISO_8859_8,   // Latin/Hebrew alphabet
        &DVBCharTableSingleByte::RAW_ISO_8859_9,   // Latin-5, Latin/Turkish alphabet
        &DVBCharTableSingleByte::RAW_ISO_8859_6,   // Latin/Arabic alphabet
        &DVBCharTableSingleByte::RAW_ISO_8859_11,  // Latin/Thai alphabet
        // 2-byte leading character table code.
        &DVBCharTableSingleByte::RAW_ISO_8859_1,   // West European alphabet
        &DVBCharTableSingleByte::RAW_ISO_8859_2,   // East European alphabet
        &DVBCharTableSingleByte::RAW_ISO_8859_3,   // South European alphabet
        &DVBCharTableSingleByte::RAW_ISO_8859_4,   // North and North-East European alphabet
        // 1-byte leading character table code.
        &ts::DVBCharTableUTF8::RAW_UTF_8,          // Last chance, can encode any string, used when no other match
    };
    return charsets;
}
