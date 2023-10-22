//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractTransportListTable.h"
#include "tsBinaryTable.h"
#include "tsSection.h"
#include "tsPSIBuffer.h"


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AbstractTransportListTable::AbstractTransportListTable(TID tid_,
                                                           const UChar* xml_name,
                                                           Standards standards,
                                                           uint16_t tid_ext_,
                                                           uint8_t version_,
                                                           bool is_current_) :
    AbstractLongTable(tid_, xml_name, standards, version_, is_current_),
    descs(this),
    transports(this),
    _tid_ext(tid_ext_)
{
}

ts::AbstractTransportListTable::AbstractTransportListTable(const AbstractTransportListTable& other) :
    AbstractLongTable(other),
    descs(this, other.descs),
    transports(this, other.transports),
    _tid_ext(other._tid_ext)
{
}

ts::AbstractTransportListTable::AbstractTransportListTable(DuckContext& duck,
                                                           TID tid,
                                                           const UChar* xml_name,
                                                           Standards standards,
                                                           const BinaryTable& table) :
    AbstractLongTable(tid, xml_name, standards, 0, true),
    descs(this),
    transports(this)
{
    deserialize(duck, table);
}

ts::AbstractTransportListTable::Transport::Transport(const AbstractTable* table) :
    EntryWithDescriptors(table)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::AbstractTransportListTable::tableIdExtension() const
{
    return _tid_ext;
}


//----------------------------------------------------------------------------
// Clear all fields.
//----------------------------------------------------------------------------

void ts::AbstractTransportListTable::clearContent()
{
    _tid_ext = 0xFFFF;
    descs.clear();
    transports.clear();
}


//----------------------------------------------------------------------------
// Clear preferred section in all transports.
//----------------------------------------------------------------------------

void ts::AbstractTransportListTable::clearPreferredSections()
{
    for (auto& it : transports) {
        it.second.preferred_section = -1;
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AbstractTransportListTable::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    _tid_ext = section.tableIdExtension();

    // Get top-level descriptor list
    buf.getDescriptorListWithLength(descs);

    // Transport stream loop.
    buf.skipReservedBits(4);
    buf.pushReadSizeFromLength(12); // transport_stream_loop_length
    while (buf.canRead()) {
        const uint16_t tsid = buf.getUInt16();
        const uint16_t nwid = buf.getUInt16();
        const TransportStreamId id(tsid, nwid);
        buf.getDescriptorListWithLength(transports[id].descs);
        transports[id].preferred_section = section.sectionNumber();
    }
    buf.popState(); // transport_stream_loop_length
}


//----------------------------------------------------------------------------
// Add a new section to a table being serialized, while inside transport loop.
//----------------------------------------------------------------------------

void ts::AbstractTransportListTable::addSection(BinaryTable& table, PSIBuffer& payload, bool last_section) const
{
    // The read/write state was pushed just before transport_stream_loop_length.

    // Update transport_stream_loop_length.
    const size_t end = payload.currentWriteByteOffset();
    payload.swapState();
    assert(payload.currentWriteByteOffset() + 2 <= end);
    const size_t loop_length = end - payload.currentWriteByteOffset() - 2;
    payload.putBits(0xFF, 4);
    payload.putBits(loop_length, 12);
    payload.popState();

    // Add the section and reset buffer.
    addOneSection(table, payload);

    // Prepare for the next section if necessary.
    if (!last_section) {
        // Empty (zero-length) top-level descriptor list.
        payload.putUInt16(0xF000);

        // Reserve transport_stream_loop_length.
        payload.pushState();
        payload.putUInt16(0xF000);
    }
}


//----------------------------------------------------------------------------
// Select a transport stream for serialization in current section.
//----------------------------------------------------------------------------

bool ts::AbstractTransportListTable::getNextTransport(TransportStreamIdSet& ts_set, TransportStreamId& ts_id, int section_number) const
{
    // Search one TS which should be serialized in current section
    for (const auto& it : ts_set) {
        if (transports[it].preferred_section == section_number) {
            ts_id = it;
            ts_set.erase(it);
            return true;
        }
    }

    // No transport for this section.
    // Search one TS without section hint or with a previous section hint.
    for (const auto& it : ts_set) {
        if (transports[it].preferred_section < section_number) { // including preferred_section == -1
            ts_id = it;
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

void ts::AbstractTransportListTable::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Build a set of TS id to serialize.
    TransportStreamIdSet ts_set;
    for (auto& tp : transports) {
        ts_set.insert(tp.first);
    }

    // Minimum size of a section: empty top-level descriptor list and transport_stream_loop_length.
    constexpr size_t payload_min_size = 4;

    // Add top-level descriptor list.
    // If the descriptor list is too long to fit into one section, create new sections when necessary.
    for (size_t start = 0;;) {
        // Reserve and restore 2 bytes for transport_stream_loop_length.
        buf.pushWriteSize(buf.size() - 2);
        start = buf.putPartialDescriptorListWithLength(descs, start);
        buf.popState();

        if (buf.error() || start >= descs.size()) {
            // Top-level descriptor list completed.
            break;
        }
        else {
            // There are remaining top-level descriptors, flush current section.
            // Add a zero transport_stream_loop_length.
            buf.putUInt16(0xF000);
            addOneSection(table, buf);
        }
    }

    // Reserve transport_stream_loop_length.
    buf.pushState();
    buf.putUInt16(0xF000);

    // Add all transports
    while (!ts_set.empty()) {

        // If we cannot at least add the fixed part of a transport, open a new section
        if (buf.remainingWriteBytes() < 6) {
            addSection(table, buf, false);
        }

        // Get a TS to serialize in current section.
        // The current section is the next one to add in the binary table.
        TransportStreamId ts_id;
        while (!getNextTransport(ts_set, ts_id, int(table.sectionCount()))) {
            // No transport found for this section, close it and starts a new one.
            addSection(table, buf, false);
        }

        // Locate transport description.
        const auto ts_iter = transports.find(ts_id);
        assert(ts_iter != transports.end());
        const DescriptorList& dlist(ts_iter->second.descs);

        // Binary size of the transport entry.
        const size_t entry_size = 6 + dlist.binarySize();

        // If we are not at the beginning of the transport loop, make sure that the
        // entire transport description fits in the section. If it does not fit,
        // start a new section. Huge transport descriptions may not fit into
        // one section, even when starting at the beginning of the transport loop.
        // In that case, the transport description will span two sections later.
        if (entry_size > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
            // Push back the transport in the set, we won't use it in this section.
            ts_set.insert(ts_id);
            // Create a new section
            addSection(table, buf, false);
            // Loop back since the section number has changed and a new transport may be better
            continue;
        }

        // Serialize the characteristics of the transport. When the section is
        // not large enough to hold the entire descriptor list, open a
        // new section for the rest of the descriptors. In that case, the
        // common properties of the transport must be repeated.
        size_t start_index = 0;
        for (;;) {
            // Insert common characteristics of the transport.
            buf.putUInt16(ts_id.transport_stream_id);
            buf.putUInt16(ts_id.original_network_id);

            // Insert descriptors (all or some).
            start_index = buf.putPartialDescriptorListWithLength(dlist, start_index);

            // Exit loop when all descriptors were serialized.
            if (start_index >= dlist.count()) {
                break;
            }

            // Not all descriptors were written, the section is full.
            // Open a new one and continue with this transport.
            addSection(table, buf, false);
        }
    }

    // Add partial section.
    addSection(table, buf, true);
}
