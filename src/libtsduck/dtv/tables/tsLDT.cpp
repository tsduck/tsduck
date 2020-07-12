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

#include "tsLDT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"LDT"
#define MY_CLASS ts::LDT
#define MY_TID ts::TID_LDT
#define MY_PID ts::PID_LDT
#define MY_STD ts::Standards::ISDB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::LDT::LDT(uint8_t vers, bool cur) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, cur),
    original_service_id(0),
    transport_stream_id(0),
    original_network_id(0),
    descriptions(this)
{
}

ts::LDT::LDT(const LDT& other) :
    AbstractLongTable(other),
    original_service_id(other.original_service_id),
    transport_stream_id(other.transport_stream_id),
    original_network_id(other.original_network_id),
    descriptions(this, other.descriptions)
{
}

ts::LDT::LDT(DuckContext& duck, const BinaryTable& table) :
    LDT()
{
    deserialize(duck, table);
}

ts::LDT::Description::Description(const AbstractTable* table) :
    EntryWithDescriptors(table)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::LDT::tableIdExtension() const
{
    return original_service_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::LDT::clearContent()
{
    original_service_id = 0;
    transport_stream_id = 0;
    original_network_id = 0;
    descriptions.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::LDT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Clear table content
    descriptions.clear();

    // Loop on all sections
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();

        // Abort if not expected table or payload too short.
        if (sect.tableId() != _table_id || remain < 4) {
            return;
        }

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        original_service_id = sect.tableIdExtension();
        transport_stream_id = GetUInt16(data);
        original_network_id = GetUInt16(data + 2);
        data += 4; remain -= 4;

        // Loop across all descriptions
        while (remain >= 5) {
            const uint16_t id = GetUInt16(data);
            size_t length = GetUInt16(data + 3) & 0x0FFF;
            data += 5; remain -= 5;

            length = std::min(length, remain);
            descriptions[id].descs.add(data, length);
            data += length; remain -= length;
        }

        // Abort in case of extra data in section.
        if (remain != 0) {
            return;
        }
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::LDT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the sections
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    int section_number = 0;
    uint8_t* data = payload;
    size_t remain = sizeof(payload);

    // Add fixed part (4 bytes)
    PutUInt16(data, transport_stream_id);
    PutUInt16(data + 2, original_network_id);
    data += 4; remain -= 4;

    // Add all descriptions.
    for (auto it = descriptions.begin(); it != descriptions.end(); ++it) {

        // If we cannot at least add the fixed part of a description, open a new section
        if (remain < 5) {
            addSection(table, section_number, payload, data, remain);
        }

        // If we are not at the beginning of the description loop, make sure that the entire
        // description fits in the section. If it does not fit, start a new section. Huge
        // descriptions may not fit into one section, even when starting at the beginning
        // of the description loop. In that case, the description will span two sections later.
        const DescriptorList& dlist(it->second.descs);
        if (data > payload + 4 && 5 + dlist.binarySize() > remain) {
            // Create a new section
            addSection(table, section_number, payload, data, remain);
        }

        // Serialize the characteristics of the description. When the section is not large enough to hold
        // the entire descriptor list, open a new section for the rest of the descriptors. In that case,
        // the common properties of the description must be repeated.
        for (size_t start_index = 0; ; ) {
            // Insert common characteristics of the description (ie. description id).
            assert(remain >= 5);
            PutUInt16(data, it->first);
            PutUInt8(data + 2, 0xFF); // reserved
            data += 3; remain -= 3;

            // Insert descriptors (all or some).
            start_index = dlist.lengthSerialize(data, remain, start_index);

            // Exit loop when all descriptors were serialized.
            if (start_index >= dlist.count()) {
                break;
            }

            // Not all descriptors were written, the section is full.
            // Open a new one and continue with this transport.
            addSection(table, section_number, payload, data, remain);
        }
    }

    // Add partial section.
    addSection(table, section_number, payload, data, remain);
}


//----------------------------------------------------------------------------
// Add a new section to a table being serialized.
//----------------------------------------------------------------------------

void ts::LDT::addSection(BinaryTable& table, int& section_number, uint8_t* payload, uint8_t*& data, size_t& remain) const
{
    table.addSection(new Section(_table_id,
                                 true,                    // is_private_section
                                 original_service_id,     // ts id extension
                                 version,
                                 is_current,
                                 uint8_t(section_number),
                                 uint8_t(section_number), //last_section_number
                                 payload,
                                 data - payload));        // payload_size,

    // Reinitialize pointers after fixed part of payload (4 bytes).
    remain += data - payload - 4;
    data = payload + 4;
    section_number++;
}


//----------------------------------------------------------------------------
// A static method to display a LDT section.
//----------------------------------------------------------------------------

void ts::LDT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    strm << margin << UString::Format(u"Original service id: 0x%X (%d)", {section.tableIdExtension(), section.tableIdExtension()}) << std::endl;

    if (size >= 4) {
        // Display common information
        strm << margin << UString::Format(u"Transport stream id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
             << margin << UString::Format(u"Original network id: 0x%X (%d)", {GetUInt16(data + 2), GetUInt16(data + 2)}) << std::endl;
        data += 4; size -= 4;

        // Loop across all descriptions
        while (size >= 5) {
            strm << margin << UString::Format(u"Description id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl;
            size_t length = GetUInt16(data + 3) & 0x0FFF;
            data += 5; size -= 5;
            if (length > size) {
                length = size;
            }
            display.displayDescriptorList(section, data, length, indent);
            data += length; size -= length;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::LDT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"original_service_id", original_service_id, true);
    root->setIntAttribute(u"transport_stream_id", transport_stream_id, true);
    root->setIntAttribute(u"original_network_id", original_network_id, true);

    for (auto it = descriptions.begin(); it != descriptions.end(); ++it) {
        xml::Element* e = root->addElement(u"description");
        e->setIntAttribute(u"description_id", it->first, true);
        it->second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::LDT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xdescriptions;
    bool ok =
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute<uint16_t>(original_service_id, u"original_service_id", true) &&
        element->getIntAttribute<uint16_t>(transport_stream_id, u"transport_stream_id", true) &&
        element->getIntAttribute<uint16_t>(original_network_id, u"original_network_id", true) &&
        element->getChildren(xdescriptions, u"description");

    for (auto it = xdescriptions.begin(); ok && it != xdescriptions.end(); ++it) {
        uint16_t id;
        ok = (*it)->getIntAttribute<uint16_t>(id, u"description_id", true) &&
             descriptions[id].descs.fromXML(duck, *it);
    }
    return ok;
}
