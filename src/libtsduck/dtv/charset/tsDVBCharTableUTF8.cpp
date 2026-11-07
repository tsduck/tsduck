//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVBCharTableUTF8.h"
#include "tsUString.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Constructor and static instances.
//----------------------------------------------------------------------------

// UTF-8 character set singleton
const ts::DVBCharTableUTF8 ts::DVBCharTableUTF8::RAW_UTF_8(u"RAW-UTF-8");
const ts::DVBCharset ts::DVBCharTableUTF8::DVB_UTF_8(u"UTF-8", RAW_UTF_8);

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

size_t ts::DVBCharTableUTF8::encode(uint8_t*& out_buffer, size_t& out_size, const UString& in_str16, size_t in_start, size_t in_count) const
{
    size_t out_count = 0;
    in_start = std::min(in_start, in_str16.length());
    in_count = std::min(in_str16.length() - in_start, in_count);

    // Serialize characters as long as there is free space.
    while (out_buffer != nullptr && out_size > 0 && in_count > 0) {
        size_t len16 = 1;
        if (in_str16[in_start] != ts::CARRIAGE_RETURN) {

            // Convert a 1-character string to UTF-8. First, identify the UTF-16 character.
            bool valid = true;
            if (IsLeadingSurrogate(in_str16[in_start])) {
                // If the character is the first one of a surrogate pair, convert the pair.
                if (in_count > 1 && IsTrailingSurrogate(in_str16[in_start+1])) {
                    len16 = 2;
                }
                else {
                    // Invalid leading part of a surrogate pair. Skip that character.
                    valid = false;
                }
            }

            // Second, convert the 1-character string.
            if (valid) {
                const std::string utf8(in_str16.substr(in_start, len16).toUTF8());
                const size_t len8 = utf8.length();
                if (len8 > out_size) {
                    // Won't fit in the buffer, stop now.
                    break;
                }

                // Small optimization.
                if (len8 == 1) {
                    *out_buffer = uint8_t(utf8[0]);
                }
                else {
                    MemCopy(out_buffer, utf8.data(), len8);
                }
                out_buffer += len8;
                out_size -= len8;
            }
        }
        out_count += len16; // include CR characters, not physically encoded, but still taken into account.
        in_start += len16;
        in_count -= len16;
    }
    return out_count;
}
