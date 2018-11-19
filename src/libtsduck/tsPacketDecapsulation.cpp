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

    // when PLAIN encapsulation is used corresponds to PUSI;
    // and when using the PES encapsulation it's an internal flag.
    bool start_mark = false;

    // A special case may arise when one original PES packet is fragmented
    // and the pointer to the next inernal packet points to a position in the
    // second part of the packet. This offset solves the problem.
    // However, it's good to overcome the fragmentation!
    uint8_t pes_fragment = 0;

    // Differentiate whether it's a plain encapsulation or a PES encapsulation
    if (!pkt.getTEI() && pkt.isClear() && pkt.hasPayload()) { // General checks
        if (pkt.getPUSI() && pktIndex < (PKT_SIZE - 9) &&
            pkt.b[pktIndex]   == 0x00 &&
            pkt.b[pktIndex+1] == 0x00 &&
            pkt.b[pktIndex+2] == 0x01) {
            // PES header found, continue...
            pktIndex += 3;

            // Check for correct Type Signature
            if (pkt.b[pktIndex++] != 0xBD) {
                return lostSync(pkt, u"invalid PES packet, type differs");
            }
            // Private Stream 1 found, continue...

            // Check for PES Size and valid Flags
            if (pkt.b[pktIndex++] != 0x00) {
                return lostSync(pkt, u"invalid PES packet, size incompatible");
            }
            const size_t pes_size = pkt.b[pktIndex++];
            // 178 bytes is the maximum PES packet size in origin.
            // However, if an external processor splits the packet and inserts
            // some PES header data (like PTS marks), then the size increases.
            // We see PES lengths of 189, but a more conservative value is used.
            if (pes_size > 255 || pes_size < 18) {
                return lostSync(pkt, u"invalid PES packet, wrong size");
            }
            if (pkt.b[pktIndex] != 0x80 && pkt.b[pktIndex] != 0x84) {
                return lostSync(pkt, u"invalid PES packet, incorrect flags");
            }
            pktIndex++; // Ignore these flags
            pktIndex++; // Ignore these flags
            size_t readed_size = pkt.b[pktIndex++]; // PES header size
            if (readed_size > 0) {
                // Advance up to the end of the PES header
                pktIndex += readed_size;
                pes_fragment = readed_size; // When fragmentation appears in the outer packet, this offset is added to checks.
            }
            // PES header OK!

            // Start reading KLVA data...
            if (pktIndex > PKT_SIZE - 18) {
                return lostSync(pkt, u"invalid PES packet, data unknown");
            }
            // Check for our KLV correct KEY
            // UL Used: 060E2B34.01010101.0F010800.0F0F0F0F
            // This is an Unique ID in the testing range.
            if (pkt.b[pktIndex]    != 0x06 && pkt.b[pktIndex+1]  != 0x0E && pkt.b[pktIndex+2]  != 0x2B && pkt.b[pktIndex+3]  != 0x34 &&
                pkt.b[pktIndex+4]  != 0x01 && pkt.b[pktIndex+5]  != 0x01 && pkt.b[pktIndex+6]  != 0x01 && pkt.b[pktIndex+7]  != 0x01 &&
                pkt.b[pktIndex+8]  != 0x0F && pkt.b[pktIndex+9]  != 0x01 && pkt.b[pktIndex+10] != 0x08 && pkt.b[pktIndex+11] != 0x00 &&
                pkt.b[pktIndex+12] != 0x0F && pkt.b[pktIndex+13] != 0x0F && pkt.b[pktIndex+14] != 0x0F && (pkt.b[pktIndex+15] != 0x0F || pkt.b[pktIndex+15] != 0x1F)
                ) {
                return lostSync(pkt, u"invalid PES packet, incorrect UL Signature");
            }
            // KLV KEY OK, continue...
            pktIndex += 16;
            start_mark = pkt.b[pktIndex-1] & 0x10; // Get the equivalent PUSI flag from the last UL Key byte.

            // Check for KLV correct LENGTH
            readed_size = pkt.b[pktIndex++];
            if (readed_size > 127 && readed_size != 0x81) {
                return lostSync(pkt, u"invalid PES packet, incorrect KLVA size");
            }
            if (readed_size > 127) readed_size = pkt.b[pktIndex++]; // BER long mode
            // KLV LENGTH OK, continue...

            // Check for KLV correct VALUE
            // No check here, this is the Data/Payload

            // Warning: We assume that each packet is a complete PES packet.
            // One special case is when an external processor has changed this,
            // and here the PES packet is delivered in multiple TS packets!
            // Following check breaks this case, so it's removed; as we can
            // continue after this point with PUSI flag off like with plain
            // encapsulation.
            //#if (readed_size + pktIndex != PKT_SIZE) {
            //#    return lostSync(pkt, u"invalid PES packet, KLVA payload doesn't match");
            //#}
            if (readed_size > 188) {
                return lostSync(pkt, u"invalid PES packet, KLVA payload doesn't match");
            }

            // At this point ALL checks are OK!
            // We assume this is a valid PES envelope.
            }
        else {
            // We assume it's a PLAIN encapsultation.
            start_mark = pkt.getPUSI();
        }
    } else return lostSync(pkt, u"incorrect packet");

    // From this point the PES envelope is consumed (therefore transparent).
    // We continue with the the PLAIN encapsulation.

    // Get pointer field when INIT MARK appears.
    const size_t pointerField = start_mark && pktIndex < PKT_SIZE ? pkt.b[pktIndex++] : 0;
    if (start_mark && pktIndex + pointerField > PKT_SIZE + pes_fragment) {
    // "pes_fragment" offset solves pointer overflows in fragmented outer packets.
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
        if (start_mark) { // PUSI mark
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
