//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsAbstractTable.h"
#include "tsBinaryTable.h"
#include "tsSection.h"
#include "tsDuckContext.h"
#include "tsPSIBuffer.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::AbstractTable::AbstractTable(TID tid, const UChar* xml_name, Standards standards, const UChar* xml_legacy_name) :
    AbstractSignalization(xml_name, standards, xml_legacy_name),
    _table_id(tid)
{
}

ts::AbstractTable::~AbstractTable()
{
}


//----------------------------------------------------------------------------
// Check if the table is a private one (ie. not MPEG-defined).
// The default implementation returns true.
// MPEG-defined tables should override this method to return false.
//----------------------------------------------------------------------------

bool ts::AbstractTable::isPrivate() const
{
    return true;
}


//----------------------------------------------------------------------------
// Get the maximum size in bytes of the payload of sections of this table.
//----------------------------------------------------------------------------

size_t ts::AbstractTable::maxPayloadSize() const
{
    return isPrivate() ? MAX_PRIVATE_SHORT_SECTION_PAYLOAD_SIZE : MAX_PSI_SHORT_SECTION_PAYLOAD_SIZE;
}


//----------------------------------------------------------------------------
// Entry base class implementation.
//----------------------------------------------------------------------------

ts::AbstractTable::EntryWithDescriptors::EntryWithDescriptors(const AbstractTable* table) :
    descs(table)
{
}

ts::AbstractTable::EntryWithDescriptors::EntryWithDescriptors(const AbstractTable* table, const EntryWithDescriptors& other) :
    descs(table, other.descs)
{
}

ts::AbstractTable::EntryWithDescriptors& ts::AbstractTable::EntryWithDescriptors::operator=(const EntryWithDescriptors& other)
{
    if (&other != this) {
        // Copying the descriptor list preserves the associated table of the target.
        descs = other.descs;
    }
    return *this;
}

ts::AbstractTable::EntryWithDescriptors& ts::AbstractTable::EntryWithDescriptors::operator=(EntryWithDescriptors&& other) noexcept
{
    if (&other != this) {
        // Moving the descriptor list preserves the associated table of the target.
        descs = std::move(other.descs);
    }
    return *this;
}


//----------------------------------------------------------------------------
// This method checks if a table id is valid for this object.
//----------------------------------------------------------------------------

bool ts::AbstractTable::isValidTableId(TID tid) const
{
    // The default implementation checks that the TID is identical to the TID of this object.
    return tid == _table_id;
}


//----------------------------------------------------------------------------
// This method serializes a table.
//----------------------------------------------------------------------------

void ts::AbstractTable::serialize(DuckContext& duck, BinaryTable& table) const
{
    // Reinitialize table object.
    table.clear();

    // Return an empty invalid table if this object is not valid.
    if (!isValid()) {
        return;
    }

    // Call the subclass implementation.
    serializeContent(duck, table);

    // Add the standards of the serialized table into the context.
    duck.addStandards(definingStandards());
}


//----------------------------------------------------------------------------
// Helper method for serializePayload(): add a section in a binary table
//----------------------------------------------------------------------------

void ts::AbstractTable::addOneSection(BinaryTable& table, PSIBuffer& payload) const
{
    // In case of error in the buffer, do not use it, do not reset it.
    if (!payload.error()) {

        // Actually add the sections.
        addOneSectionImpl(table, payload);

        // Reset the payload buffer
        if (payload.pushedReadWriteStateLevels() > 0) {
            // At least one read/write state is pushed, restore it and push it again.
            payload.popReadWriteState();
            payload.pushReadWriteState();
        }
        else {
            // No saved state, reset payload buffer.
            payload.readSeek(0);
            payload.writeSeek(0);
        }
    }
}

void ts::AbstractTable::addOneSectionImpl(BinaryTable &table, PSIBuffer &payload) const
{
    // Always set one single section in short tables.
    if (table.sectionCount() == 0) {
        const SectionPtr section(new Section(tableId(), isPrivate(), payload.currentReadAddress(), payload.remainingReadBytes()));
        table.addSection(section, true);
    }
    else {
        // More than one section, this is an error.
        payload.setUserError();
    }
}


//----------------------------------------------------------------------------
// This method deserializes a binary table.
//----------------------------------------------------------------------------

void ts::AbstractTable::deserialize(DuckContext& duck, const BinaryTable& table)
{
    // Make sure the object is cleared before analyzing the binary table.
    clear();

    // Keep this object invalid if the binary table is invalid or has an incorrect table if for this class.
    if (!table.isValid() || !isValidTableId(table.tableId())) {
        invalidate();
        return;
    }

    // Table is already checked to be compatible but can be different from current one.
    // So, we need to update this object.
    _table_id = table.tableId();

    // Call the subclass implementation.
    deserializeContent(duck, table);

    // Add the standards of the deserialized table into the context.
    duck.addStandards(definingStandards());
}


//----------------------------------------------------------------------------
// Wrapper for deserializePayload (overridden by intermediate classes).
//----------------------------------------------------------------------------

void ts::AbstractTable::deserializePayloadWrapper(PSIBuffer& buf, const Section& section)
{
    // At this level, we directly invoke the subclass handler.
    deserializePayload(buf, section);
}


//----------------------------------------------------------------------------
// Default implementations for serialization handlers.
// Will disappear some day when refactoring is complete...
//----------------------------------------------------------------------------

void ts::AbstractTable::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build a buffer of the appropriate size.
    PSIBuffer payload(duck, maxPayloadSize());

    // Let the subclass serialize the sections payloads.
    serializePayload(table, payload);

    // Upon return, add unfinished section when necessary.
    if (payload.error()) {
        // There were serialization errors, invalidate the binary table.
        table.clear();
    }
    else if (table.sectionCount() == 0) {
        // No section were added, add this one, even if empty.
        addOneSection(table, payload);
    }
    else {
        // Some sections were already added. Check if we need to add a last one.
        // By default, we add it if it is not empty.
        bool add = payload.remainingReadBytes() > 0;
        // But if there is a saved read/write state and nothing was added since the saved state,
        // then we assume that the saved state is fixed initial common data, identical in all
        // sections, and there is no need to add the last section.
        if (add && payload.pushedReadWriteStateLevels() > 0) {
            const size_t current_write = payload.currentWriteByteOffset();
            payload.swapReadWriteState();
            add = current_write > payload.currentWriteByteOffset();
            payload.swapReadWriteState();
        }
        // Finally, add the section if necessary.
        if (add) {
            addOneSection(table, payload);
        }
    }
}

void ts::AbstractTable::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Loop on all sections in the table.
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // The table is already valid.
        const Section& section(*table.sectionAt(si));
        assert(section.isValid());

        // Map a deserialization read-only buffer over the payload part.
        PSIBuffer buf(duck, section.payload(), section.payloadSize());

        // Let the subclass deserialize the payload in the buffer.
        // We call it through a wrapper virtual method to let intermediate classes
        // (typically AbstractLongTable) extract common fields.
        deserializePayloadWrapper(buf, section);

        if (buf.error() || !buf.endOfRead()) {
            // Deserialization error or extraneous data, not a valid section.
            clear();
            invalidate();
            break;
        }
    }
}

void ts::AbstractTable::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Generate an error to invalidate the deserialization.
    buf.setUserError();
}

void ts::AbstractTable::serializePayload(BinaryTable& table, PSIBuffer& payload) const
{
    // Generate an error to invalidate the serialization.
    payload.setUserError();
}
