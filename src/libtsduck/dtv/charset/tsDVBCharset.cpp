//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBCharset.h"
#include "tsUString.h"
#include "tsDVBCharTableSingleByte.h"
#include "tsDVBCharTableUTF8.h"

// Default predefined DVB character set (using ISO-6937 as default table).
const ts::DVBCharset ts::DVBCharset::DVB(u"DVB");


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::DVBCharset::DVBCharset(const UChar* name, const DVBCharTable* default_table) :
    Charset(name),
    _default_table(default_table != nullptr ? default_table : &DVBCharTableSingleByte::RAW_ISO_6937)
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
    const DVBCharTable* table = code == 0 ? _default_table : DVBCharTable::GetTableFromLeadingCode(code);
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

    // Try to encode using these character tables in order
    const DVBCharTable* const lookup_tables[] = {
        _default_table,                                // default table for this charset
        &ts::DVBCharTableSingleByte::RAW_ISO_6937,     // default DVB table, same as previous in most cases
        &ts::DVBCharTableSingleByte::RAW_ISO_8859_15,  // most european characters and Euro currency sign
        &ts::DVBCharTableUTF8::RAW_UTF_8,              // last chance, used when no other match
        nullptr                                        // end of list
    };

    // Look for a character set which can encode the string.
    const DVBCharTable* table = nullptr;
    for (size_t i = 0; lookup_tables[i] != nullptr; ++i) {
        if ((i == 0 || lookup_tables[i] != _default_table) && lookup_tables[i]->canEncode(str, start, count)) {
            table = lookup_tables[i];
            break;
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
