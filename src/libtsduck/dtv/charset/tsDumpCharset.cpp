//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDumpCharset.h"
#include "tsUString.h"

// Default predefined character set.
const ts::DumpCharset ts::DumpCharset::DUMP(u"DUMP");


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::DumpCharset::DumpCharset(const UChar* name) :
    Charset(name)
{
}


//----------------------------------------------------------------------------
// Decode a string from the specified byte buffer.
// Return an hexadecimal representation of the string.
//----------------------------------------------------------------------------

bool ts::DumpCharset::decode(UString& str, const uint8_t* data, size_t size) const
{
    str = UString::Dump(data, size, UString::SINGLE_LINE);
    return true;
}


//----------------------------------------------------------------------------
// Check if a string can be encoded using the charset.
// The string to "encode" shall contain an even number of hexadecimal bytes.
// Spaces are ignored.
//----------------------------------------------------------------------------

bool ts::DumpCharset::canEncode(const UString& str, size_t start, size_t count) const
{
    const size_t len = str.length();
    const size_t end = count > len ? len : std::min(len, start + count);

    size_t hexcount = 0;
    for (size_t i = start; i < end; ++i) {
        const UChar c = str[i];
        if (IsHexa(c)) {
            hexcount++;
        }
        else if (!IsSpace(c)) {
            return false;
        }
    }
    return hexcount % 2 == 0;
}


//----------------------------------------------------------------------------
// Encode a C++ Unicode string into a string.
//----------------------------------------------------------------------------

size_t ts::DumpCharset::encode(uint8_t*& buffer, size_t& size, const UString& str, size_t start, size_t count) const
{
    if (buffer == nullptr) {
        return 0;
    }

    const size_t len = str.length();
    const size_t end = count > len ? len : std::min(len, start + count);

    size_t after_encode = start;  // index after last encoded character
    bool got_nibble = false;      // already got the first nibble of a byte
    int nibble = 0;               // first nibble of current byte value to encode

    for (size_t i = start; i < end && size > 0; ++i) {
        const UChar c = str[i];
        if (IsHexa(c)) {
            if (got_nibble) {
                // Now build the encoded byte.
                *buffer++ = uint8_t((nibble << 4) | ToDigit(c, 16));
                size--;
                after_encode = i + 1;
                got_nibble = false;
            }
            else {
                // Get first nibble of next byte.
                nibble = ToDigit(c, 16);
                got_nibble = true;
            }
        }
        else if (!IsSpace(c)) {
            // Stop at first non-encodable character.
            break;
        }
        else if (!got_nibble) {
            // Include / skip inter-bytes spaces in encoded characters.
            after_encode = i + 1;
        }
    }

    // Return number of encoded bytes
    return after_encode - start;
}
