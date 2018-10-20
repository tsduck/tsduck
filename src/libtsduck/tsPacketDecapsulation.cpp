//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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

#include "tsPacketDecapsulation.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::PacketDecapsulation::PacketDecapsulation(PID pid) :
    _pidInput(pid),
    _synchronized(false),
    _ccInput(0),
    _nextIndex(1),
    _nextPacket(),
    _lastError()
{
    // There is always implicitely one sync byte in decapsulated packets.
    _nextPacket.b[0] = SYNC_BYTE;
}


//----------------------------------------------------------------------------
// Reset the decapsulation.
//----------------------------------------------------------------------------

void ts::PacketDecapsulation::reset(PID pid)
{
    _pidInput = pid;
    _synchronized = false;
    _nextIndex = 1; // after sync byte
    _lastError.clear();
}


//----------------------------------------------------------------------------
// Lose synchronization, return false.
//----------------------------------------------------------------------------

bool ts::PacketDecapsulation::lostSync(const UString& error)
{
    _synchronized = false;
    _nextIndex = 1;  // after sync byte
    _lastError = error;
    return false;
}

bool ts::PacketDecapsulation::lostSync(TSPacket& pkt, const UString& error)
{
    pkt = NullPacket;  // return a null packet since nothing was decapsulated
    return lostSync(error);
}


//----------------------------------------------------------------------------
// Process a TS packet from the input stream.
//----------------------------------------------------------------------------

bool ts::PacketDecapsulation::processPacket(ts::TSPacket& pkt)
{
    // Work on the input PID only.
    if (_pidInput == PID_NULL || pkt.getPID() != _pidInput) {
        return true;
    }

    // Where to look at in input packet. Start at beginning of payload.
    size_t pktIndex = pkt.getHeaderSize();

    // Get pointer field in PUSI packets.
    const bool pusi = pkt.getPUSI();
    const size_t pointerField = pusi && pktIndex < PKT_SIZE ? pkt.b[pktIndex++] : 0;
    if (pusi && pktIndex + pointerField > PKT_SIZE) {
        return lostSync(pkt, u"invalid packet, adaptation field or pointer field out of range");
    }

    // Check continuity counter.
    const uint8_t cc = pkt.getCC();
    if (_synchronized && cc != ((_ccInput + 1) & CC_MASK)) {
        // Got a discontinuity, lose synchronization but will maybe resync later, do not return an error.
        lostSync(u"input PID discontinuity");
    }
    _ccInput = cc;

    // If we previously lost synchronization, try to resync in current packet.
    if (!_synchronized) {
        if (pusi) {
            // There is a packet start here, we have a chance to resync.
            pktIndex += pointerField;
            _synchronized = true;
        }
        else {
            // We cannot resync now, simply return a null packet.
            pkt = NullPacket;
            return true;
        }
    }

    // Copy data in next packet.
    assert(pktIndex <= PKT_SIZE);
    assert(_nextIndex <= PKT_SIZE);
    size_t size = std::min(PKT_SIZE - pktIndex, PKT_SIZE - _nextIndex);
    ::memcpy(_nextPacket.b + _nextIndex, pkt.b + pktIndex, size);
    pktIndex += size;
    _nextIndex += size;

    if (_nextIndex == PKT_SIZE) {
        // Next packet is full, return it.
        const TSPacket tmp(pkt);
        pkt = _nextPacket;
        // Copy start of next packet.
        size = PKT_SIZE - pktIndex;
        ::memcpy(_nextPacket.b + 1, tmp.b + pktIndex, size);
        _nextIndex = 1 + size;
    }
    else {
        // Next packet not full, must have exhausted the input packet.
        assert(pktIndex == PKT_SIZE);
        assert(_nextIndex < PKT_SIZE);
        // Replace input packet with a null packet since we cannot extract a packet now.
        pkt = NullPacket;
    }

    return true;
}
