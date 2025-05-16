//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard (PES mode by lars18th)
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPacketDecapsulation.h"
#include "tsMemory.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::PacketDecapsulation::PacketDecapsulation(Report& report, PID pid) :
    _report(report),
    _input_pid(pid)
{
}


//----------------------------------------------------------------------------
// Reset the decapsulation.
//----------------------------------------------------------------------------

void ts::PacketDecapsulation::reset(PID pid)
{
    _packet_count = 0;
    _input_pid = pid;
    _synchronized = false;
    _next_index = 1; // after sync byte
    _last_error.clear();
}


//----------------------------------------------------------------------------
// Lose synchronization, return false.
//----------------------------------------------------------------------------

bool ts::PacketDecapsulation::lostSync(const UString& error)
{
    _synchronized = false;
    _next_index = 1;  // after sync byte
    _last_error = error;
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
    // Count all processed packets, not only from the input PID.
    _packet_count++;

    // Work on the input PID only.
    if (_input_pid == PID_NULL || pkt.getPID() != _input_pid) {
        return true;
    }

    // Where to look at in input packet. Start at beginning of payload.
    size_t pkt_index = pkt.getHeaderSize();

    // when PLAIN encapsulation is used it corresponds to PUSI;
    // and when using the PES encapsulation it's an internal flag.
    bool start_mark = false;

    // A special case may arise when one original PES packet is fragmented
    // and the pointer to the next inernal packet points to a position in the
    // second part of the packet. This offset solves the problem.
    // However, it's good to overcome the fragmentation!
    size_t pes_fragment = 0;

    // Used to distinguish between ASYNC and SYNC PES encapsulation.
    bool pes_sync = false;

    // Differentiate whether it's a plain encapsulation or a PES encapsulation
    if (!pkt.getTEI() && pkt.isClear() && pkt.hasPayload()) { // General checks

        if (pkt.getPUSI() && pkt_index < (PKT_SIZE - 9) &&
            pkt.b[pkt_index]   == 0x00 &&
            pkt.b[pkt_index+1] == 0x00 &&
            pkt.b[pkt_index+2] == 0x01)
        {
            // PES header found, continue...
            pkt_index += 3;

            // Check for correct Type Signature of the PES packet
            if (pkt.b[pkt_index] != 0xBD && pkt.b[pkt_index] != 0xFC) {
                return lostSync(pkt, u"invalid PES packet, type differs");
            }
            else if (pkt.b[pkt_index] == 0xFC) {
                pes_sync = true;
            }
            // (ASYNC=Private Stream 1 || SYNC=Metadata Stream) found, continue...
            pkt_index++;

            // Check for PES Size (2 bytes)
            if (pkt.b[pkt_index++] != 0x00) {
                return lostSync(pkt, u"invalid PES packet, size incompatible");
            }
            const size_t pes_size = pkt.b[pkt_index++];
            // 178 bytes is the maximum PES packet size in origin.
            // However, if an external processor splits the packet and inserts
            // some PES header data (like PTS marks), then the size increases.
            // We see PES lengths of 189, but a more conservative value is used.
            if (pes_size > 255 || pes_size < 18) {
                return lostSync(pkt, u"invalid PES packet, wrong size");
            }

            // Check for valid flags
            if ((pkt.b[pkt_index]   != 0x80 && pkt.b[pkt_index]   != 0x84) ||
                (pkt.b[pkt_index+1] != 0x80 && pkt.b[pkt_index+1] != 0x00)) {
                return lostSync(pkt, u"invalid PES packet, incorrect flags");
            }
            pkt_index++; // Ignore these flags
            pkt_index++; // Ignore these flags

            // Check remaining header
            size_t header_size = pkt.b[pkt_index++]; // PES optional header size (1 byte)
            if (header_size > 0) {
                // Advance up to the end of the PES header
                pkt_index += header_size;
                pes_fragment = header_size; // When fragmentation appears in the outer packet, this offset is added to checks.
            }
            // PES header OK!

            // Check Metadata AU Header (5 bytes), only in Synchronous mode.
            if (pes_sync) {
               if (pkt.b[pkt_index] != 0x00 || pkt.b[pkt_index+2] != 0xDF) {
                   return lostSync(pkt, u"invalid PES packet, SYNC Metadata Header incorrect");
               }
               if (pkt.b[pkt_index+3] != 0x00 || pkt.b[pkt_index+4] > 206) {
                   return lostSync(pkt, u"invalid PES packet, SYNC AU cell data size incompatible");
               }
               pkt_index += 5;
            }

            // Start reading KLVA data...
            if (pkt_index > PKT_SIZE - 18) {
                return lostSync(pkt, u"invalid PES packet, data unknown");
            }
            // Check for our KLV correct KEY
            // UL Used: 060E2B34.01010101.0F010800.0F0F0F0F
            // This is an Unique ID in the testing range.
            if (pkt.b[pkt_index]    != 0x06 || pkt.b[pkt_index+1]  != 0x0E || pkt.b[pkt_index+2]  != 0x2B || pkt.b[pkt_index+3]  != 0x34 ||
                pkt.b[pkt_index+4]  != 0x01 || pkt.b[pkt_index+5]  != 0x01 || pkt.b[pkt_index+6]  != 0x01 || pkt.b[pkt_index+7]  != 0x01 ||
                pkt.b[pkt_index+8]  != 0x0F || pkt.b[pkt_index+9]  != 0x01 || pkt.b[pkt_index+10] != 0x08 || pkt.b[pkt_index+11] != 0x00 ||
                pkt.b[pkt_index+12] != 0x0F || pkt.b[pkt_index+13] != 0x0F || pkt.b[pkt_index+14] != 0x0F || (pkt.b[pkt_index+15] != 0x0F && pkt.b[pkt_index+15] != 0x1F))
            {
                return lostSync(pkt, u"invalid PES packet, incorrect UL Signature");
            }
            // KLV KEY OK, continue...
            pkt_index += 16;
            start_mark = pkt.b[pkt_index-1] & 0x10; // Get the equivalent PUSI flag from the last UL Key byte.

            // Check for KLV correct LENGTH
            size_t read_length = pkt.b[pkt_index++];
            if (read_length > 127 && read_length != 0x81) {
                return lostSync(pkt, u"invalid PES packet, incorrect KLVA size");
            }
            if (read_length > 127) {
                read_length = pkt.b[pkt_index++]; // BER long mode with 2 bytes
            }
            // KLV LENGTH OK, continue...

            // Check for KLV correct VALUE
            // No check here, this is the Data/Payload

            // Warning: We assume that each packet is a complete PES packet.
            // One special case is when an external processor has changed this,
            // and here the PES packet is delivered in multiple TS packets!
            // Following check breaks this case, so it's removed; as we can
            // continue after this point with PUSI flag off like with plain
            // encapsulation.
            //#if (readed_length + pktIndex != PKT_SIZE) {
            //#    return lostSync(pkt, u"invalid PES packet, KLVA payload doesn't match");
            //#}
            if (read_length > 188) {
                return lostSync(pkt, u"invalid PES packet, KLVA payload doesn't match");
            }

            // At this point ALL checks are OK!
            // We assume that this is a valid PES envelope and
            // the remainig data is the encapsulated packet.
        }
        else {
            // We assume it's a PLAIN encapsultation.
            start_mark = pkt.getPUSI();
        }
    }
    else {
        return lostSync(pkt, u"incorrect packet");
    }

    // From this point the PES envelope is consumed (therefore transparent).
    // We continue with the the PLAIN encapsulation.

    // Get pointer field when INIT MARK appears.
    const size_t pointer_field = start_mark && pkt_index < PKT_SIZE ? pkt.b[pkt_index++] : 0;
    if (start_mark && pkt_index + pointer_field > PKT_SIZE + pes_fragment) {
        // "pes_fragment" offset solves pointer overflows in fragmented outer packets.
        return lostSync(pkt, u"invalid packet, adaptation field or pointer field out of range");
    }

    // Check continuity counter.
    const uint8_t cc = pkt.getCC();
    if (_synchronized && cc != ((_cc_input + 1) & CC_MASK)) {
        // Got a discontinuity, lose synchronization but will maybe resync later, do not return an error.
        lostSync(u"input PID discontinuity");
    }
    _cc_input = cc;

    // If we previously lost synchronization, try to resync in current packet.
    if (!_synchronized) {
        if (start_mark) { // PUSI mark
            // There is a packet start here, we have a chance to resync.
            pkt_index += pointer_field;
            _synchronized = true;
        }
        else {
            // We cannot resync now, simply return a null packet.
            pkt = NullPacket;
            return true;
        }
    }

    // Copy data in next packet.
    assert(pkt_index <= PKT_SIZE);
    assert(_next_index <= PKT_SIZE);
    size_t size = std::min(PKT_SIZE - pkt_index, PKT_SIZE - _next_index);
    MemCopy(_next_packet.b + _next_index, pkt.b + pkt_index, size);
    pkt_index += size;
    _next_index += size;

    if (_next_index == PKT_SIZE) {
        // Next packet is full, return it.
        const TSPacket tmp(pkt);
        pkt = _next_packet;
        // Copy start of next packet.
        size = PKT_SIZE - pkt_index;
        MemCopy(_next_packet.b + 1, tmp.b + pkt_index, size);
        _next_index = 1 + size;
    }
    else {
        // Next packet not full, must have exhausted the input packet.
        assert(pkt_index == PKT_SIZE);
        assert(_next_index < PKT_SIZE);
        // Replace input packet with a null packet since we cannot extract a packet now.
        pkt = NullPacket;
    }

    return true;
}
