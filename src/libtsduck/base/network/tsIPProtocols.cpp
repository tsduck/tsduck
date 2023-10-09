//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
