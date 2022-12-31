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

#include "tsIPProtocols.h"
#include "tsNamesFile.h"


//----------------------------------------------------------------------------
// Get the name of an IP protocol (UDP, TCP, etc).
//----------------------------------------------------------------------------

ts::UString ts::IPProtocolName(uint8_t protocol, bool long_format)
{
    // The strings in tsduck.ip.names use format "acronym: description".
    UString name(ts::NamesFile::Instance(ts::NamesFile::Predefined::IP)->nameFromSection(u"IPProtocol", protocol));
    if (!long_format) {
        const size_t colon = name.find(u':');
        if (colon != NPOS) {
            name.resize(colon);
        }
    }
    return name;
}


//----------------------------------------------------------------------------
// TCP sequence numbers.
//----------------------------------------------------------------------------

bool ts::TCPOrderedSequence(uint32_t seq1, uint32_t seq2)
{
    constexpr size_t may_wrap = 0xFFFFFFFF - TCP_MAX_PAYLOAD_SIZE;
    return (seq1 < may_wrap) ? (seq1 < seq2) : (seq1 < seq2 || seq1 - seq2 > may_wrap);
}

uint32_t ts::TCPSequenceDiff(uint32_t seq1, uint32_t seq2)
{
    // In fact, the modular arithmetics does this transparently but it is
    // better to have it in a separate function, at least to enforce modular
    // arithmetics on uint32_t.
    return seq2 - seq1;
}
