//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBCharTableUTF16.h"
#include "tsMemory.h"
#include "tsUString.h"


//----------------------------------------------------------------------------
// Constructor and static instances.
//----------------------------------------------------------------------------

const ts::DVBCharTableUTF16 ts::DVBCharTableUTF16::RAW_UNICODE(u"RAW-UNICODE");
const ts::DVBCharset ts::DVBCharTableUTF16::DVB_UNICODE(u"UNICODE", &RAW_UNICODE);

ts::DVBCharTableUTF16::DVBCharTableUTF16(const UChar* name) :
    DVBCharTable(name, 0x000011)
{
}


//----------------------------------------------------------------------------
// Decode a DVB string from the specified byte buffer.
//----------------------------------------------------------------------------

bool ts::DVBCharTableUTF16::decode(UString& str, const uint8_t* dvb, size_t dvbSize) const
{
    // We simply copy 2 bytes per character.
    str.clear();
    str.reserve(dvbSize / 2);
    for (size_t i = 0; dvb != nullptr && i + 1 < dvbSize; i += 2) {
        const uint16_t cp = GetUInt16(dvb + i);
        str.push_back(cp == DVB_CODEPOINT_CRLF ? ts::LINE_FEED : UChar(cp));
    }

    // Truncated string if odd number of bytes.
    return dvbSize % 2 == 0;
}


//----------------------------------------------------------------------------
// Check if a string can be encoded using the charset.
//----------------------------------------------------------------------------

bool ts::DVBCharTableUTF16::canEncode(const UString& str, size_t start, size_t count) const
{
    // All characters and can always be encoded in UTF-16.
    return true;
}


//----------------------------------------------------------------------------
// Encode a C++ Unicode string into a DVB string.
//----------------------------------------------------------------------------

size_t ts::DVBCharTableUTF16::encode(uint8_t*& buffer, size_t& size, const UString& str, size_t start, size_t count) const
{
    size_t result = 0;
    // Serialize characters as long as there is free space.
    while (buffer != nullptr && size > 1 && start < str.length() && count > 0) {
        const UChar cp = str[start];
        if (cp != ts::CARRIAGE_RETURN) {
            // Encode character.
            PutUInt16(buffer, cp == ts::LINE_FEED ? DVB_CODEPOINT_CRLF : uint16_t(cp));
            buffer += 2;
            size -= 2;
        }
        result++; // include CR characters, not physically encoded, but still taken into account.
        start++;
        count--;
    }
    return result;
}
