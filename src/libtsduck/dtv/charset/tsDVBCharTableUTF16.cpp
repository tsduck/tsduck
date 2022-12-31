//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
            result++;
        }
        start++;
        count--;
    }
    return result;
}
