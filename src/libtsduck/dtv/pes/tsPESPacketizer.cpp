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

#include "tsPESPacketizer.h"
#include "tsTSPacket.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PESPacketizer::PESPacketizer(const DuckContext& duck, PID pid, PESProviderInterface* provider) :
    AbstractPacketizer(duck, pid),
    _provider(provider),
    _pes(nullptr),
    _next_byte(0),
    _pes_out_count(0),
    _pes_in_count(0)
{
}

ts::PESPacketizer::~PESPacketizer()
{
}


//----------------------------------------------------------------------------
// Reset the content of a packetizer. Becomes empty.
//----------------------------------------------------------------------------

void ts::PESPacketizer::reset()
{
    _pes.clear();
    _next_byte = 0;
    AbstractPacketizer::reset();
}


//----------------------------------------------------------------------------
// Build the next TS packet.
//----------------------------------------------------------------------------

bool ts::PESPacketizer::getNextPacket(TSPacket& pkt)
{
    // If there is no current section, get the next one.
    if (_pes.isNull() && _provider != nullptr) {
        _provider->providePESPacket(_pes_in_count, _pes);
        _next_byte = 0;
        if (!_pes.isNull()) {
            _pes_in_count++;
        }
    }

    // If there is still no current section, return a null packet
    if (_pes.isNull()) {
        configurePacket(pkt, true);
        return false;
    }

    // Initialize a TS packet.
    pkt.init();
    configurePacket(pkt, false);

    // Special treatment for first TS packet of a PES packet.
    if (_next_byte == 0) {
        pkt.setPUSI();
        pkt.setPCR(_pes->getPCR(), true); // void if pcr == INVALID_PCR
    }

    // How much of the PES packet we can store in the TS payload.
    assert(_next_byte < _pes->size());
    const size_t count = std::min(_pes->size() - _next_byte, pkt.getPayloadSize());

    // At the end of the PES packet, there are less bytes to store than the TS payload.
    // We need to create a stuffed adaptation field in the TS packet to "push" the
    // rest of the PES packet at the end of the TS packet.
    if (count < pkt.getPayloadSize()) {
        pkt.setPayloadSize(count);
    }

    // Copy the PES data in the TS payload.
    ::memcpy(pkt.getPayload(), _pes->content() + _next_byte, count);
    _next_byte += count;

    // Get rid of current packet when completed.
    if (_next_byte >= _pes->size()) {
        _pes_out_count++;
        _next_byte = 0;
        _pes.clear();
    }
    return true;
}


//----------------------------------------------------------------------------
// Display the internal state of the packetizer, mainly for debug
//----------------------------------------------------------------------------

std::ostream& ts::PESPacketizer::display(std::ostream& strm) const
{
    return AbstractPacketizer::display(strm)
        << UString::Format(u"  Output PES packets: %'d", {_pes_out_count}) << std::endl
        << UString::Format(u"  Provided PES packets: %'d", {_pes_in_count}) << std::endl
        << UString::Format(u"  Current PES packet: offset %d/%d", {_next_byte, _pes.isNull() ? 0 : _pes->size()}) << std::endl;
}
