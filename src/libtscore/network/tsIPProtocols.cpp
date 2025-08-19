//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPProtocols.h"
#include "tsNames.h"


//----------------------------------------------------------------------------
// Get the name of an IP protocol (UDP, TCP, etc).
//----------------------------------------------------------------------------

ts::UString ts::IPProtocolName(uint8_t protocol, bool long_format)
{
    // The strings in tsduck.ip.names use format "acronym: description".
    UString name(NameFromSection(u"ip", u"IPProtocol", protocol));
    if (!long_format) {
        const size_t colon = name.find(u':');
        if (colon != NPOS) {
            name.resize(colon);
        }
    }
    return name;
}


//------------------------------------------------------------------------
// Get the standard text for a HTTP status code.
//------------------------------------------------------------------------

ts::UString ts::HTTPStatusText(int status)
{
    return NameFromSection(u"ip", u"HTTP.status", status);
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


//----------------------------------------------------------------------------
// VLAN identification.
//----------------------------------------------------------------------------

ts::UString ts::VLANIdStack::toString() const
{
    UString s;
    for (const auto& id : *this) {
        if (!s.empty()) {
            s.push_back(u'<');
        }
        s.format(u"%d", id.id);
    }
    return s;
}

bool ts::VLANIdStack::match(const VLANIdStack& other) const
{
    const size_t this_size = size();
    const size_t other_size = other.size();
    if (this_size < other_size) {
        return false;
    }
    const size_t size = std::min(this_size, other_size);
    for (size_t i = 0; i < size; ++i) {
        const VLANId& a((*this)[i]);
        const VLANId& b(other[i]);
        if ((a.type != b.type && a.type != ETHERTYPE_NULL && b.type != ETHERTYPE_NULL) ||
            (a.id != b.id && a.id != VLAN_ID_NULL && b.id != VLAN_ID_NULL))
        {
            return false;
        }
    }
    return true;
}
