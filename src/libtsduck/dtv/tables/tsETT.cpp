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

#include "tsETT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"ETT"
#define MY_CLASS ts::ETT
#define MY_TID ts::TID_ETT
#define MY_STD ts::Standards::ATSC

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ETT::ETT(uint8_t version_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, true), // ETT is always "current"
    ETT_table_id_extension(0),
    protocol_version(0),
    ETM_id(0),
    extended_text_message()
{
}

ts::ETT::ETT(DuckContext& duck, const BinaryTable& table) :
    ETT()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::ETT::tableIdExtension() const
{
    return ETT_table_id_extension;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::ETT::clearContent()
{
    ETT_table_id_extension = 0;
    protocol_version = 0;
    ETM_id = 0;
    extended_text_message.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ETT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Clear table content
    ETT_table_id_extension = 0;
    ETM_id = 0;
    protocol_version = 0;
    extended_text_message.clear();

    // An ETT is not allowed to use more than one section, see A/65, section 6.2.
    if (table.sectionCount() == 0) {
        return;
    }
    const Section& sect(*table.sectionAt(0));

    // Analyze the section payload:
    const uint8_t* data = sect.payload();
    size_t remain = sect.payloadSize();
    if (remain < 5) {
        return; // invalid table, too short
    }

    version = sect.version();
    is_current = sect.isCurrent(); // should be true
    ETT_table_id_extension = sect.tableIdExtension();

    protocol_version = data[0];
    ETM_id = GetUInt32(data + 1);
    data += 5; remain -= 5;

    _is_valid = extended_text_message.deserialize(duck, data, remain);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ETT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the section. Note that an ETT is not allowed to use more than one section, see A/65, section 6.2.
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    uint8_t* data = payload;
    size_t remain = sizeof(payload);

    data[0] = protocol_version;
    PutUInt32(data + 1, ETM_id);
    data += 5; remain -= 5;
    extended_text_message.serialize(duck, data, remain);

    // Add one single section in the table
    table.addSection(new Section(MY_TID,           // tid
                                 true,             // is_private_section
                                 ETT_table_id_extension,
                                 version,
                                 is_current,       // should be true
                                 0,                // section_number,
                                 0,                // last_section_number
                                 payload,
                                 data - payload)); // payload_size,
}


//----------------------------------------------------------------------------
// A static method to display a ETT section.
//----------------------------------------------------------------------------

void ts::ETT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    if (size >= 5) {
        // Fixed part.
        uint32_t id = GetUInt32(data + 1);
        strm << margin << UString::Format(u"ETT table id extension: 0x%X (%d)", {section.tableIdExtension(), section.tableIdExtension()}) << std::endl
             << margin << UString::Format(u"Protocol version: %d, ETM id: 0x%X (%d)", {data[0], id, id}) << std::endl;
        data += 5; size -= 5;
        ATSCMultipleString::Display(display, u"Extended text message: ", indent, data, size);
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ETT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setIntAttribute(u"protocol_version", protocol_version);
    root->setIntAttribute(u"ETT_table_id_extension", ETT_table_id_extension, true);
    root->setIntAttribute(u"ETM_id", ETM_id, true);
    extended_text_message.toXML(duck, root, u"extended_text_message", true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ETT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
           element->getIntAttribute<uint8_t>(protocol_version, u"protocol_version", false, 0) &&
           element->getIntAttribute<uint16_t>(ETT_table_id_extension, u"ETT_table_id_extension", true) &&
           element->getIntAttribute<uint32_t>(ETM_id, u"ETM_id", true) &&
           extended_text_message.fromXML(duck, element, u"extended_text_message", false);
}
