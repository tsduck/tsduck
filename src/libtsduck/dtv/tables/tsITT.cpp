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

#include "tsITT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
#include "tsMJD.h"
#include "tsBCD.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"ITT"
#define MY_CLASS ts::ITT
#define MY_TID ts::TID_ITT
#define MY_STD ts::Standards::ISDB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::ITT::ITT(uint8_t vers, bool cur) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, cur),
    event_id(0),
    descs(this)
{
}

ts::ITT::ITT(const ITT& other) :
    AbstractLongTable(other),
    event_id(other.event_id),
    descs(this, other.descs)
{
}

ts::ITT::ITT(DuckContext& duck, const BinaryTable& table) :
    ITT()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::ITT::tableIdExtension() const
{
    return event_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::ITT::clearContent()
{
    event_id = 0;
    descs.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ITT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Clear table content
    descs.clear();

    // Loop on all sections
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();

        // Abort if not expected table or payload too short.
        if (sect.tableId() != _table_id || remain < 2) {
            return;
        }

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        event_id = sect.tableIdExtension();

        // Get descriptor loop.
        size_t len = GetUInt16(data) & 0x0FFF;
        data += 2; remain -= 2;
        len = std::min(len, remain);
        descs.add(data, len);
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ITT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the sections. There is only a descriptor loop in the payload.
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    int section_number = 0;
    size_t desc_index = 0;

    do {
        uint8_t* data = payload;
        size_t remain = sizeof(payload);
        desc_index = descs.lengthSerialize(data, remain, desc_index);
        table.addSection(new Section(_table_id,
                                     true,                    // is_private_section
                                     event_id,                // ts id extension
                                     version,
                                     is_current,
                                     uint8_t(section_number),
                                     uint8_t(section_number), //last_section_number
                                     payload,
                                     data - payload));        // payload_size,
        section_number++;
    } while (desc_index < descs.count());
}


//----------------------------------------------------------------------------
// A static method to display an ITT section.
//----------------------------------------------------------------------------

void ts::ITT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    strm << margin << UString::Format(u"Event id: 0x%X (%d)", {section.tableIdExtension(), section.tableIdExtension()}) << std::endl;

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();
    size_t len = GetUInt16(data) & 0x0FFF;
    data += 2; size -= 2;
    len = std::min(len, size);
    display.displayDescriptorList(section, data, len, indent);
    data += len; size -= len;

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ITT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"event_id", event_id, true);
    descs.toXML(duck, root);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ITT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
           element->getBoolAttribute(is_current, u"current", false, true) &&
           element->getIntAttribute<uint16_t>(event_id, u"event_id", true) &&
           descs.fromXML(duck, element);
}
