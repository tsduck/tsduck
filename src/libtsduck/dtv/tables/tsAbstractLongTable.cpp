//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
    version(version_),
    is_current(is_current_)
{
}

ts::AbstractLongTable::~AbstractLongTable()
{
}


//----------------------------------------------------------------------------
// Get the maximum size in bytes of the payload of sections of this table.
//----------------------------------------------------------------------------

size_t ts::AbstractLongTable::maxPayloadSize() const
{
    return isPrivate() ? MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE : MAX_PSI_LONG_SECTION_PAYLOAD_SIZE;
}


//----------------------------------------------------------------------------
// Check if the sections of this table have a trailing CRC32.
//----------------------------------------------------------------------------

bool ts::AbstractLongTable::useTrailingCRC32() const
{
    // By default, all long sections have a CRC32.
    return true;
}


//----------------------------------------------------------------------------
// This method clears the content of the table.
//----------------------------------------------------------------------------

void ts::AbstractLongTable::clear()
{
    // Clear using superclass, including call to clearContent().
    AbstractTable::clear();

    // Clear fields of this class.
    version = 0;
    is_current = true;
}


//----------------------------------------------------------------------------
// Deserialize a section.
//----------------------------------------------------------------------------

void ts::AbstractLongTable::deserializePayloadWrapper(PSIBuffer& buf, const Section& section)
{
    // Extract fields of this class.
    version = section.version();
    is_current = section.isCurrent();

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
        const SectionPtr section(new Section(tableId(),
                                             isPrivate(),
                                             tableIdExtension(),
                                             version,
                                             is_current,
                                             section_number,
                                             section_number, // last_section_number
                                             payload.currentReadAddress(),
                                             payload.remainingReadBytes()));
        table.addSection(section, true, true);
    }
    else {
        // Too many sections, this is an error.
        payload.setUserError();
    }
}
