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
//
//  Abstract base class for tables containing a list of transport stream
//  descriptions. Common code for BAT and NIT.
//
//----------------------------------------------------------------------------

#include "tsAbstractTransportListTable.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AbstractTransportListTable::AbstractTransportListTable(TID tid_,
                                                           const UChar* xml_name,
                                                           uint16_t tid_ext_,
                                                           uint8_t version_,
                                                           bool is_current_) :
    AbstractLongTable(tid_, xml_name, version_, is_current_),
    descs(this),
    transports(this),
    _tid_ext(tid_ext_)
{
    _is_valid = true;
}

ts::AbstractTransportListTable::AbstractTransportListTable(const AbstractTransportListTable& other) :
    AbstractLongTable(other),
    descs(this, other.descs),
    transports(this, other.transports),
    _tid_ext(other._tid_ext)
{
}

ts::AbstractTransportListTable::AbstractTransportListTable(TID tid, const UChar* xml_name, const BinaryTable& table, const DVBCharset* charset) :
    AbstractLongTable(tid, xml_name),
    descs(this),
    transports(this),
    _tid_ext(0xFFFF)
{
    deserialize(table, charset);
}

ts::AbstractTransportListTable::Transport::Transport(const AbstractTable* table) :
    EntryWithDescriptors(table),
    preferred_section(-1)
{
}


//----------------------------------------------------------------------------
// Clear preferred section in all transports.
//----------------------------------------------------------------------------

void ts::AbstractTransportListTable::clearPreferredSections()
{
    for (auto it = transports.begin(); it != transports.end(); ++it) {
        it->second.preferred_section = -1;
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AbstractTransportListTable::deserialize(const BinaryTable& table, const DVBCharset* charset)
{
    // Clear table content
    _is_valid = false;
    _tid_ext = 0xFFFF;
    descs.clear();
    transports.clear();

    if (!table.isValid()) {
        return;
    }

    // Check table id: Must be the same one as set in the constructor,
    // except NIT Actual and NIT Other which can be mixed.
    const TID tid (table.tableId());
    if ((_table_id == TID_NIT_ACT || _table_id == TID_NIT_OTH) && (tid == TID_NIT_ACT || tid == TID_NIT_OTH)) {
        // Both are NITs and compatible
        _table_id = tid;
    }
    else if (tid != _table_id) {
        return;
    }

    // Loop on all sections
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Abort if not expected table
        if (sect.tableId() != _table_id) {
            return;
        }

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        _tid_ext = sect.tableIdExtension();

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();

        // Get top-level descriptor list
        if (remain < 2) {
            return;
        }
        size_t info_length (GetUInt16 (data) & 0x0FFF);
        data += 2;
        remain -= 2;
        info_length = std::min (info_length, remain);
        descs.add (data, info_length);
        data += info_length;
        remain -= info_length;

        // Get transports description length
        if (remain < 2) {
            return;
        }
        size_t ts_length (GetUInt16 (data) & 0x0FFF);
        data += 2;
        remain -= 2;
        remain = std::min (ts_length, remain);

        // Get transports description
        while (remain >= 6) {
            TransportStreamId id(GetUInt16 (data), GetUInt16 (data + 2)); // tsid, onid
            info_length = GetUInt16(data + 4) & 0x0FFF;
            data += 6;
            remain -= 6;
            info_length = std::min(info_length, remain);
            transports[id].descs.add(data, info_length);
            transports[id].preferred_section = int(si);
            data += info_length;
            remain -= info_length;
        }
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Private method: Add a new section to a table being serialized.
// Section number is incremented. Data and remain are reinitialized.
//----------------------------------------------------------------------------

void ts::AbstractTransportListTable::addSection(BinaryTable& table,
                                                int& section_number,
                                                uint8_t* payload,
                                                uint8_t*& data,
                                                size_t& remain) const
{
    table.addSection(new Section(_table_id,
                                 true,   // is_private_section
                                 _tid_ext,
                                 version,
                                 is_current,
                                 uint8_t(section_number),
                                 uint8_t(section_number),   //last_section_number
                                 payload,
                                 data - payload)); // payload_size,

    // Reinitialize pointers.
    remain += data - payload;
    data = payload;
    section_number++;
}


//----------------------------------------------------------------------------
// Private method: Same as previous, while being inside the transport loop.
//----------------------------------------------------------------------------

void ts::AbstractTransportListTable::addSection(BinaryTable& table,
                                                int& section_number,
                                                uint8_t* payload,
                                                uint8_t*& tsll_addr,
                                                uint8_t*& data,
                                                size_t& remain) const
{
    // Update transport_stream_loop_length in current section
    PutUInt16(tsll_addr, 0xF000 | uint16_t(data - tsll_addr - 2));

    // Add current section, open a new one
    addSection(table, section_number, payload, data, remain);

    // Insert a zero-length global descriptor loop
    assert(remain >= 4);
    PutUInt16(data, 0xF000);

    // Reserve transport_stream_loop_length.
    tsll_addr = data + 2;
    PutUInt16(data + 2, 0xF000);
    data += 4;
    remain -= 4;
}


//----------------------------------------------------------------------------
// Select a transport stream for serialization in current section.
// If found, set ts_id, remove the ts id from the set and return true.
// Otherwise, return false.
//----------------------------------------------------------------------------

bool ts::AbstractTransportListTable::getNextTransport(TransportStreamIdSet& ts_set,
                                                      TransportStreamId& ts_id,
                                                      int section_number) const
{
    // Search one TS which should be serialized in current section
    for (auto it = ts_set.begin(); it != ts_set.end(); ++it) {
        if (transports[*it].preferred_section == section_number) {
            ts_id = *it;
            ts_set.erase(it);
            return true;
        }
    }

    // No transport for this section.
    // Search one TS without section hint or with a previous section hint.
    for (auto it = ts_set.begin(); it != ts_set.end(); ++it) {
        if (transports[*it].preferred_section < section_number) { // including preferred_section == -1
            ts_id = *it;
            ts_set.erase(it);
            return true;
        }
    }

    // No TS found. Either no more TS in ts_set or all remaining TS have
    // a section hint for subsequent sections.
    return false;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AbstractTransportListTable::serialize(BinaryTable& table, const DVBCharset* charset) const
{
    // Reinitialize table object
    table.clear ();

    // Return an empty table if not valid
    if (!_is_valid) {
        return;
    }

    // Build a set of TS id to serialize.
    TransportStreamIdSet ts_set;
    for (TransportMap::const_iterator it = transports.begin(); it != transports.end(); ++it) {
        ts_set.insert(it->first);
    }

    // Build the sections
    uint8_t payload[MAX_PSI_LONG_SECTION_PAYLOAD_SIZE];
    int section_number(0);
    uint8_t* data(payload);
    size_t remain(sizeof(payload));

    // Add top-level descriptor list.
    // If the descriptor list is too long to fit into one section,
    // create new sections when necessary.
    for (size_t start_index = 0; ; ) {

        // Add the descriptor list (or part of it).
        // Reserve 2 extra bytes at end, for the rest of the section
        assert(remain > 2);
        remain -= 2;
        start_index = descs.lengthSerialize(data, remain, start_index);
        remain += 2;

        // If all descriptors were serialized, exit loop
        if (start_index == descs.count()) {
            break;
        }
        assert(start_index < descs.count());

        // Need to close the section and open a new one.
        // Add a zero transport_stream_loop_length.
        assert(remain >= 2);
        PutUInt16(data, 0xF000);
        data += 2;
        remain -= 2;
        addSection(table, section_number, payload, data, remain);
    }

    // Reserve transport_stream_loop_length.
    assert (remain >= 2);
    uint8_t* tsll_addr = data;
    PutUInt16 (data, 0xF000);
    data += 2;
    remain -= 2;

    // Add all transports
    while (!ts_set.empty()) {

        // If we cannot at least add the fixed part of a transport, open a new section
        if (remain < 6) {
            addSection (table, section_number, payload, tsll_addr, data, remain);
            assert (remain >= 6);
        }

        // Get a TS to serialize in current section
        TransportStreamId ts_id;
        while (!getNextTransport (ts_set, ts_id, section_number)) {
            // No transport found for this section, close it and starts a new one.
            addSection (table, section_number, payload, tsll_addr, data, remain);
        }

        // Locate transport description
        const TransportMap::const_iterator ts_iter(transports.find(ts_id));
        assert(ts_iter != transports.end());
        const DescriptorList& dlist(ts_iter->second.descs);

        // If we are not at the beginning of the transport loop, make sure that the
        // entire transport description fits in the section. If it does not fit,
        // start a new section. Huge transport descriptions may not fit into
        // one section, even when starting at the beginning of the transport loop.
        // In that case, the transport description will span two sections later.
        if (data > tsll_addr + 2 && 6 + dlist.binarySize() > remain) {
            // Push back the transport in the set
            ts_set.insert (ts_id);
            // Create a new section
            addSection (table, section_number, payload, tsll_addr, data, remain);
            // Loop back since the section number has changed and a new transport may be better
            continue;
        }

        // Serialize the characteristics of the transport. When the section is
        // not large enough to hold the entire descriptor list, open a
        // new section for the rest of the descriptors. In that case, the
        // common properties of the transport must be repeated.
        size_t start_index = 0;
        for (;;) {
            // Insert common characteristics of the transport
            assert (remain >= 6);
            PutUInt16 (data, ts_id.transport_stream_id);
            PutUInt16 (data + 2, ts_id.original_network_id);
            data += 4;
            remain -= 4;

            // Insert descriptors (all or some).
            start_index = dlist.lengthSerialize(data, remain, start_index);

            // Exit loop when all descriptors were serialized.
            if (start_index >= dlist.count()) {
                break;
            }

            // Not all descriptors were written, the section is full.
            // Open a new one and continue with this transport.
            addSection(table, section_number, payload, tsll_addr, data, remain);
        }
    }

    // Add partial section.
    addSection(table, section_number, payload, tsll_addr, data, remain);
}
