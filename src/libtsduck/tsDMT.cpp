//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  Representation of a Download Marker Table (DMT)
//  This is a Logiways private table.
//
//----------------------------------------------------------------------------

#include "tsDMT.h"
#include "tsCRC32.h"


#if defined (TS_NEED_STATIC_CONST_DEFINITIONS)
const size_t ts::DMT::MAX_ENTRIES;
#endif


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::DMT::DMT (uint32_t asset_id_, uint16_t remaining_) :
    AbstractTable (TID_LW_DMT),
    asset_id (asset_id_),
    remaining_broadcast_count (remaining_),
    time_stamp (),
    entries ()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary table
//----------------------------------------------------------------------------

ts::DMT::DMT (const BinaryTable& table) :
    AbstractTable (TID_LW_DMT),
    asset_id (0),
    remaining_broadcast_count (0),
    time_stamp (),
    entries ()
{
    deserialize (table);
}


//----------------------------------------------------------------------------
// Count current and total packets, for all components
//----------------------------------------------------------------------------

uint32_t ts::DMT::getTotalPacketCount() const
{
    uint32_t count = 0;
    for (EntryVector::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        count += it->total_packet_count;
    }
    return count;
}

uint32_t ts::DMT::getPacketCount() const
{
    uint32_t count = 0;
    for (EntryVector::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        count += it->packet_count;
    }
    return count;
}


//----------------------------------------------------------------------------
// Search for an entry matching a specified component tag.
// Return this->entries.end() if not found.
//----------------------------------------------------------------------------

ts::DMT::EntryVector::const_iterator ts::DMT::search (uint8_t component_tag) const
{
    EntryVector::const_iterator it;
    for (it = entries.begin(); it != entries.end() && it->component_tag != component_tag; ++it) {
    }
    return it;
}

ts::DMT::EntryVector::iterator ts::DMT::search (uint8_t component_tag)
{
    EntryVector::iterator it;
    for (it = entries.begin(); it != entries.end() && it->component_tag != component_tag; ++it) {
    }
    return it;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DMT::deserialize (const BinaryTable& table)
{
    // Clear table content
    _is_valid = false;
    time_stamp.reset();
    entries.clear();

    if (!table.isValid()) {
        return;
    }

    // This is a short table, must have only one section
    if (table.sectionCount() != 1) {
        return;
    }

    // Reference to single section
    const Section& sect (*table.sectionAt(0));
    const uint8_t* data = sect.payload();
    size_t remain = sect.payloadSize();

    // Abort if not a DMT
    if (sect.tableId() != _table_id || remain < 15) {
        return;
    }

    // A DMT section is a short section with a CRC32.
    // Normally, only long sections have a CRC32.
    // So, the generic code has not checked the CRC32.
    // Remove CRC32 from payload and verify it.
    remain -= 4;
    const size_t size = sect.size() - 4;
    if (CRC32 (sect.content(), size) != GetUInt32 (sect.content() + size)) {
        return;
    }

    // Get fixed part
    asset_id = GetUInt32 (data);
    remaining_broadcast_count = GetUInt16 (data + 4);
    if (data[6] & 0x02) {
        time_stamp = GetUInt64 (data + 3) & TS_UCONST64 (0x00000001FFFFFFFF);
    }
    data += 11; remain -= 11;

    // Get all component entries
    while (remain >= 9) {
        Entry entry;
        entry.component_tag = data[0];
        entry.packet_count = GetUInt32 (data + 1);
        entry.total_packet_count = GetUInt32 (data + 5);
        entries.push_back (entry);
        data += 9; remain -= 9;
    }

    _is_valid = remain == 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DMT::serialize (BinaryTable& table) const
{
    // Reinitialize table object
    table.clear();

    // Return an empty table if not valid
    if (!_is_valid) {
        return;
    }

    // Build the section
    uint8_t payload [MAX_PSI_SHORT_SECTION_PAYLOAD_SIZE];
    uint8_t* data = payload;
    size_t remain = sizeof(payload) - 4; // Keep 4 bytes for CRC.

    // Serialize the section
    PutUInt32 (data, asset_id);
    PutUInt16 (data + 4, remaining_broadcast_count);
    data[6] = time_stamp.set() ? (0xFE | uint8_t ((time_stamp.value() >> 32) & 0x01)) : 0xFD;
    PutUInt32 (data + 7, uint32_t (time_stamp.value (0xFFFFFFFF)));
    data += 11; remain -= 11;

    for (EntryVector::const_iterator it = entries.begin(); it != entries.end() && remain >= 9; ++it) {
        data[0] = it->component_tag;
        PutUInt32 (data + 1, it->packet_count);
        PutUInt32 (data + 5, it->total_packet_count);
        data += 9; remain -= 9;
    }

    // Add the section in the table (including CRC placeholder)
    table.addSection (new Section (TID_LW_DMT,
                                   true,   // is_private_section
                                   payload,
                                   data + 4 - payload));

    // Now artificially rebuild a CRC at end of section
    const Section& sect (*table.sectionAt(0));
    data = const_cast <uint8_t*> (sect.content());
    size_t size = sect.size();
    assert (size > 4);
    PutUInt32 (data + size - 4, CRC32 (data, size - 4).value());
}


//----------------------------------------------------------------------------
// Write a DMT into one TS packet.
// The TS header (PID, continuity counter, etc) is not modified.
//----------------------------------------------------------------------------

void ts::DMT::serialize (TSPacket& pkt, PID pid, uint8_t cc) const
{
    assert (entries.size() <= MAX_ENTRIES);

    // TS header
    pkt.b[0] = SYNC_BYTE;
    PutUInt16 (pkt.b + 1, 0x4000 | (pid & 0x1FFF)); // PUSI, PID
    pkt.b[3] = 0x10 | (cc & 0x0F);                  // has payload, CC

    // TS payload, section header, section fixed part
    pkt.b[4] = 0x00;                 // pointer field
    uint8_t* section_addr = pkt.b + 5; // section start
    pkt.b[5] = TID_LW_DMT;           // table_id
    uint8_t* length_addr = pkt.b + 6;  // section_length addr
    PutUInt32 (pkt.b + 8, asset_id);
    PutUInt16 (pkt.b + 12, remaining_broadcast_count);
    pkt.b[14] = time_stamp.set() ? (0xFE | uint8_t ((time_stamp.value() >> 32) & 0x01)) : 0xFD;
    PutUInt32 (pkt.b + 15, uint32_t (time_stamp.value(0xFFFFFFFF)));

    // Section variable part
    uint8_t* data = pkt.b + 19;

    for (EntryVector::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        data[0] = it->component_tag;
        PutUInt32 (data + 1, it->packet_count);
        PutUInt32 (data + 5, it->total_packet_count);
        data += 9;
        assert (data < pkt.b + PKT_SIZE - 4);
    }

    // Update section length (including CRC to come)
    PutUInt16 (length_addr, 0x7000 | ((data - length_addr + 2) & 0x0FFF));

    // CRC at end of section
    PutUInt32 (data, CRC32 (section_addr, data - section_addr).value());
    data += 4;

    // Section stuffing
    ::memset (data, 0xFF, pkt.b + PKT_SIZE - data);
}
