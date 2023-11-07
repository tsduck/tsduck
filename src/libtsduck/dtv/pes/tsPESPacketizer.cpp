//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPESPacketizer.h"
#include "tsTSPacket.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PESPacketizer::PESPacketizer(const DuckContext& duck, PID pid, PESProviderInterface* provider) :
    AbstractPacketizer(duck, pid),
    _provider(provider)
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
    std::memcpy(pkt.getPayload(), _pes->content() + _next_byte, count);
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
