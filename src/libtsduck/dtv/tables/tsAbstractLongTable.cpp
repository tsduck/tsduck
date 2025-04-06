//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractLongTable.h"
#include "tsPSIBuffer.h"
#include "tsBinaryTable.h"
#include "tsSection.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::AbstractLongTable::AbstractLongTable(TID tid, const UChar* xml_name, Standards standards, uint8_t version_, bool is_current_) :
    AbstractTable(tid, xml_name, standards),
    _version(version_),
    _is_current(is_current_)
{
}

ts::AbstractLongTable::~AbstractLongTable()
{
}


//----------------------------------------------------------------------------
// Default implementations of simple virtual methods.
//----------------------------------------------------------------------------

// Get the maximum size in bytes of the payload of sections of this table.
size_t ts::AbstractLongTable::maxPayloadSize() const
{
    return isPrivate() ? MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE : MAX_PSI_LONG_SECTION_PAYLOAD_SIZE;
}

// Check if the sections of this table have a trailing CRC32.
bool ts::AbstractLongTable::useTrailingCRC32() const
{
    // By default, all long sections have a CRC32.
    return true;
}

// Get the table version.
uint8_t ts::AbstractLongTable::version() const
{
    return _version;
}

// Set the table version.
void ts::AbstractLongTable::setVersion(uint8_t version)
{
    _version = version & SVERSION_MASK;
}

// Check if the table is current.
bool ts::AbstractLongTable::isCurrent() const
{
    return _is_current;
}

// Set the current/next status of the table.
void ts::AbstractLongTable::setCurrent(bool is_current)
{
    _is_current = is_current;
}


//----------------------------------------------------------------------------
// This method clears the content of the table.
//----------------------------------------------------------------------------

void ts::AbstractLongTable::clear()
{
    // Clear using superclass, including call to clearContent().
    AbstractTable::clear();

    // Clear fields of this class.
    setVersion(0);
    setCurrent(true);
}


//----------------------------------------------------------------------------
// Deserialize a section.
//----------------------------------------------------------------------------

void ts::AbstractLongTable::deserializePayloadWrapper(PSIBuffer& buf, const Section& section)
{
    // Extract fields of this class.
    setVersion(section.version());
    setCurrent(section.isCurrent());

    // Deserialize using superclass, including call to deserializePayload().
    AbstractTable::deserializePayloadWrapper(buf, section);
}


//----------------------------------------------------------------------------
// Add a section in a binary table
//----------------------------------------------------------------------------

void ts::AbstractLongTable::addOneSectionImpl(BinaryTable& table, PSIBuffer& payload) const
{
    // Always add a new section, after last one, in long tables.
    if (table.sectionCount() < 256) {
        // Add one section.
        const uint8_t section_number = uint8_t(table.sectionCount());
        table.addNewSection(tableId(),
                            isPrivate(),
                            tableIdExtension(),
                            _version,
                            _is_current,
                            section_number,
                            section_number, // last_section_number
                            payload.currentReadAddress(),
                            payload.remainingReadBytes());
    }
    else {
        // Too many sections, this is an error.
        payload.setUserError();
    }
}
