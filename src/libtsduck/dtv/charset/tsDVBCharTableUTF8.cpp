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
                ::memcpy(buffer, utf8.data(), len);
            }
            buffer += len;
            size -= len;
            result++;
        }
        start++;
        count--;
    }
    return result;
}
