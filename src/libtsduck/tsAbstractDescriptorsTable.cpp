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
//  Base class for MPEG tables containing only a list of descriptors.
//
//----------------------------------------------------------------------------

#include "tsAbstractDescriptorsTable.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::AbstractDescriptorsTable::AbstractDescriptorsTable(TID tid_, const char* xml_name, uint16_t tid_ext_, uint8_t version_, bool is_current_) :
    AbstractLongTable(tid_, xml_name, version_, is_current_),
    descs(),
    _tid_ext(tid_ext_)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary table
//----------------------------------------------------------------------------

ts::AbstractDescriptorsTable::AbstractDescriptorsTable(TID tid, const char* xml_name, const BinaryTable& table) :
    AbstractLongTable(tid, xml_name),
    descs(),
    _tid_ext(0xFFFF)
{
    deserialize(table);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AbstractDescriptorsTable::deserialize (const BinaryTable& table)
{
    // Clear table content
    _is_valid = false;
    descs.clear ();

    if (!table.isValid() || table.tableId() != _table_id) {
        return;
    }

    // Loop on all sections
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect (*table.sectionAt(si));

        // Get common properties
        version = sect.version();
        is_current = sect.isCurrent();
        _tid_ext = sect.tableIdExtension();

        // Analyze the section payload:
        const uint8_t* data (sect.payload());
        size_t remain (sect.payloadSize());

        // Get descriptor list
        descs.add (data, remain);
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AbstractDescriptorsTable::serialize (BinaryTable& table) const
{
    // Reinitialize table object
    table.clear ();

    // Return an empty table if not valid
    if (!_is_valid) {
        return;
    }

    // Add all descriptors, creating several sections if necessary.
    // Make sure to create at least one section if the list is empty.

    uint8_t payload [MAX_PSI_LONG_SECTION_PAYLOAD_SIZE];
    int section_number (0);
    uint8_t* data (payload);
    size_t remain (sizeof(payload));
    size_t start_index (0);

    while ((section_number == 0 || start_index < descs.count()) && section_number < 256) {

        // Serialize as much descriptors as possible
        start_index = descs.serialize (data, remain, start_index);

        // Add section in the table
        table.addSection (new Section (_table_id,
                                       false,  // is_private_section
                                       _tid_ext,
                                       version,
                                       is_current,
                                       uint8_t(section_number),
                                       uint8_t(section_number), // last_section_number
                                       payload,
                                       data - payload));        // payload_size,

        // Prepare for next section (if any)
        section_number++;
        data = payload;
        remain = sizeof(payload);
    }
}


//----------------------------------------------------------------------------
// A static method to display a section.
//----------------------------------------------------------------------------

void ts::AbstractDescriptorsTable::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    display.displayDescriptorList(section.payload(), section.payloadSize(), indent, section.tableId());
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::AbstractDescriptorsTable::toXML(XML& xml, XML::Document& doc) const
{
    return 0; // TODO @@@@
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::AbstractDescriptorsTable::fromXML(XML& xml, const XML::Element* element)
{
    // TODO @@@@
}
