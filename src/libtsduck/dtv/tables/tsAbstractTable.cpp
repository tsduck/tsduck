//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractTable.h"
#include "tsBinaryTable.h"
#include "tsSection.h"
#include "tsDuckContext.h"
#include "tsPSIBuffer.h"
#include "tsCRC32.h"


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
// Checks if a table id is valid for this object.
//----------------------------------------------------------------------------

bool ts::AbstractTable::isValidTableId(TID tid) const
{
    // The default implementation checks that the TID is identical to the TID of this object.
    // Subclasses for which several table ids are valid should override this method.
    return tid == _table_id;
}


//----------------------------------------------------------------------------
// Check if the table is a private one (ie. not MPEG-defined).
//----------------------------------------------------------------------------

bool ts::AbstractTable::isPrivate() const
{
    // The default implementation returns true.
    // MPEG-defined tables should override this method to return false.
    return true;
}


//----------------------------------------------------------------------------
// Get the maximum size in bytes of the payload of sections of this table.
//----------------------------------------------------------------------------

size_t ts::AbstractTable::maxPayloadSize() const
{
    // The default implementation returns the size of short sections payload.
    // AbstractLongTable should override this with the size of long sections payload.
    return isPrivate() ? MAX_PRIVATE_SHORT_SECTION_PAYLOAD_SIZE : MAX_PSI_SHORT_SECTION_PAYLOAD_SIZE;
}


//----------------------------------------------------------------------------
// Check if the sections of this table have a trailing CRC32.
//----------------------------------------------------------------------------

bool ts::AbstractTable::useTrailingCRC32() const
{
    // By default, short sections do not use a CRC32.
    return false;
}


//----------------------------------------------------------------------------
// Entry base class implementation.
//----------------------------------------------------------------------------

ts::AbstractTable::EntryWithDescriptors::EntryWithDescriptors(const AbstractTable* table) :
    EntryBase(NPOS),
    descs(table)
{
}

ts::AbstractTable::EntryWithDescriptors::EntryWithDescriptors(const AbstractTable* table, const EntryWithDescriptors& other) :
    EntryBase(NPOS),
    descs(table, other.descs)
{
}

ts::AbstractTable::EntryWithDescriptors::EntryWithDescriptors(const AbstractTable* table, EntryWithDescriptors&& other) :
    EntryBase(NPOS),
    descs(table, other.descs)
{
}

ts::AbstractTable::EntryWithDescriptors& ts::AbstractTable::EntryWithDescriptors::operator=(const EntryWithDescriptors& other)
{
    if (&other != this) {
        // Copying the descriptor list preserves the associated table of the target.
        descs = other.descs;
        order_hint = other.order_hint;
    }
    return *this;
}

ts::AbstractTable::EntryWithDescriptors& ts::AbstractTable::EntryWithDescriptors::operator=(EntryWithDescriptors&& other) noexcept
{
    if (&other != this) {
        // Moving the descriptor list preserves the associated table of the target.
        descs = std::move(other.descs);
        order_hint = other.order_hint;
    }
    return *this;
}


//----------------------------------------------------------------------------
// This method serializes a table.
//----------------------------------------------------------------------------

bool ts::AbstractTable::serialize(DuckContext& duck, BinaryTable& table) const
{
    // Reinitialize table object.
    table.clear();

    // Return an empty invalid table if this object is not valid.
    if (!isValid()) {
        return false;
    }

    // Add the standards of the serialized table into the context.
    duck.addStandards(definingStandards());

    // Build a buffer of the appropriate size.
    PSIBuffer payload(duck, maxPayloadSize());

    // Let the subclass serialize the sections payloads.
    serializePayload(table, payload);

    // Upon return, add unfinished section when necessary.
    if (payload.error()) {
        // There were serialization errors, invalidate the binary table.
        table.clear();
        return false;
    }
    else if (table.sectionCount() == 0) {
        // No section were added, add this one, even if empty.
        addOneSection(table, payload);
        return true;
    }
    else {
        // Some sections were already added. Check if we need to add a last one.
        // By default, we add it if it is not empty.
        bool add = payload.remainingReadBytes() > 0;
        // But if there is a saved read/write state and nothing was added since the saved state,
        // then we assume that the saved state is fixed initial common data, identical in all
        // sections, and there is no need to add the last section.
        if (add && payload.pushedLevels() > 0) {
            const size_t current_write = payload.currentWriteByteOffset();
            payload.swapState();
            add = current_write > payload.currentWriteByteOffset();
            payload.swapState();
        }
        // Finally, add the section if necessary.
        if (add) {
            addOneSection(table, payload);
        }
        return !payload.error();
    }
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
        if (payload.pushedLevels() > 0) {
            // At least one read/write state is pushed, restore it and push it again.
            payload.popState();
            payload.pushState();
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
    // This is the implementation for short tables.
    // This method is overridden in AbstractLongTable.
    // Always set one single section in short tables.
    if (table.sectionCount() == 0) {
        const SectionPtr section(new Section(tableId(), isPrivate(), payload.currentReadAddress(), payload.remainingReadBytes()));
        // Add a trailing CRC32 if this table needs it, even though this is a short section.
        if (useTrailingCRC32()) {
            // The CRC must be computed on the section with the final CRC included in the length.
            section->appendPayload(ByteBlock(4));
            section->setUInt32(section->payloadSize() - 4, CRC32(section->content(), section->size() - 4));
        }
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

bool ts::AbstractTable::deserialize(DuckContext& duck, const BinaryTable& table)
{
    // Make sure the object is cleared before analyzing the binary table.
    clear();

    // Keep this object invalid if the binary table is invalid or has an incorrect table if for this class.
    if (!table.isValid() || !isValidTableId(table.tableId())) {
        invalidate();
        return false;
    }

    // Table is already checked to be compatible but can be different from current one.
    // So, we need to update this object.
    _table_id = table.tableId();

    // Loop on all sections in the table.
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // The binary table is already valid, so its sectiosn are valid too.
        const Section& section(*table.sectionAt(si));
        assert(section.isValid());

        // Check if we shall manually check the value of a CRC32 in a short section.
        const bool short_crc = section.isShortSection() && useTrailingCRC32();
        if (short_crc) {
            // This is a short section which needs a CRC32.
            if (section.size() < 4 || CRC32(section.content(), section.size() - 4) != GetUInt32(section.content() + section.size() - 4)) {
                // Invalid CRC32, not a valid section.
                clear();
                invalidate();
                break;
            }
        }

        // Map a deserialization read-only buffer over the payload part.
        // Remove CRC32 from payload in short sections that have one.
        PSIBuffer buf(duck, section.payload(), section.payloadSize() - (short_crc ? 4 : 0));

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

    // Add the standards of the deserialized table into the context.
    duck.addStandards(definingStandards());
    return isValid();
}


//----------------------------------------------------------------------------
// Wrapper for deserializePayload (overridden by intermediate classes).
//----------------------------------------------------------------------------

void ts::AbstractTable::deserializePayloadWrapper(PSIBuffer& buf, const Section& section)
{
    // At this level, we directly invoke the subclass handler.
    deserializePayload(buf, section);
}
