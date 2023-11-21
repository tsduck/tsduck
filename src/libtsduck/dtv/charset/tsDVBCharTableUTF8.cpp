//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBCharTableUTF8.h"
#include "tsUString.h"


//----------------------------------------------------------------------------
// Constructor and static instances.
//----------------------------------------------------------------------------

// UTF-8 character set singleton
const ts::DVBCharTableUTF8 ts::DVBCharTableUTF8::RAW_UTF_8(u"RAW-UTF-8");
const ts::DVBCharset ts::DVBCharTableUTF8::DVB_UTF_8(u"UTF-8", &RAW_UTF_8);

ts::DVBCharTableUTF8::DVBCharTableUTF8(const UChar* name) :
    DVBCharTable(name, 0x000015)
{
}


//----------------------------------------------------------------------------
// Decode a DVB string from the specified byte buffer.
//----------------------------------------------------------------------------

bool ts::DVBCharTableUTF8::decode(UString& str, const uint8_t* dvb, size_t dvbSize) const
{
    str = UString::FromUTF8(reinterpret_cast<const char*>(dvb), dvbSize);
    return true;
}


//----------------------------------------------------------------------------
// Check if a string can be encoded using the charset.
//----------------------------------------------------------------------------

bool ts::DVBCharTableUTF8::canEncode(const UString& str, size_t start, size_t count) const
{
    // All characters and can always be encoded in UTF-8.
    return true;
}


//----------------------------------------------------------------------------
// Encode a C++ Unicode string into a DVB string.
//----------------------------------------------------------------------------

size_t ts::DVBCharTableUTF8::encode(uint8_t*& buffer, size_t& size, const UString& str, size_t start, size_t count) const
{
    size_t result = 0;

    // Serialize characters as long as there is free space.
    while (buffer != nullptr && size > 0 && start < str.length() && count > 0) {
        if (str[start] != ts::CARRIAGE_RETURN) {

            // Convert a 1-character string to UTF-8.
            const std::string utf8(str.substr(start, 1).toUTF8());
            const size_t len = utf8.length();
            if (len > size) {
                // Won't fit in the buffer, stop now.
                break;
            }

            // Small optimization.
            if (len == 1) {
                *buffer = uint8_t(utf8[0]);
            }
            else {
                std::memcpy(buffer, utf8.data(), len);
            }
            buffer += len;
            size -= len;
        }
        result++; // include CR characters, not physically encoded, but still taken into account.
        start++;
        count--;
    }
    return result;
}
