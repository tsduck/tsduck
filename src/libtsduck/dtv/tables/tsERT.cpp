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

#include "tsERT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"ERT"
#define MY_CLASS ts::ERT
#define MY_TID ts::TID_ERT
#define MY_PID ts::PID_ERT
#define MY_STD ts::Standards::ISDB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::ERT::ERT(uint8_t vers, bool cur) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, cur),
    event_relation_id(0),
    information_provider_id(0),
    relation_type(0),
    relations(this)
{
}

ts::ERT::ERT(const ERT& other) :
    AbstractLongTable(other),
    event_relation_id(other.event_relation_id),
    information_provider_id(other.information_provider_id),
    relation_type(other.relation_type),
    relations(this, other.relations)
{
}

ts::ERT::ERT(DuckContext& duck, const BinaryTable& table) :
    ERT()
{
    deserialize(duck, table);
}

ts::ERT::Relation::Relation(const AbstractTable* table) :
    EntryWithDescriptors(table),
    node_id(0),
    collection_mode(0),
    parent_node_id(0),
    reference_number(0)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::ERT::tableIdExtension() const
{
    return event_relation_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::ERT::clearContent()
{
    event_relation_id = 0;
    information_provider_id = 0;
    relation_type = 0;
    relations.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ERT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Clear table content
    relations.clear();

    // Loop on all sections
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();

        // Abort if not expected table or payload too short.
        if (sect.tableId() != _table_id || remain < 3) {
            return;
        }

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        event_relation_id = sect.tableIdExtension();
        information_provider_id = GetUInt16(data);
        relation_type = (data[2] >> 4) & 0x0F;
        data += 3; remain -= 3;

        // Loop across all local events.
        while (remain >= 8) {

            // Get fixed part.
            Relation& rel(relations.newEntry());
            rel.node_id = GetUInt16(data);
            rel.collection_mode = (data[2] >> 4) & 0x0F;
            rel.parent_node_id = GetUInt16(data + 3);
            rel.reference_number = data[5];
            size_t len = GetUInt16(data + 6) & 0x0FFF;
            data += 8; remain -= 8;

            if (len > remain) {
                return;
            }

            // Get descriptor loop.
            rel.descs.add(data, len);
            data += len; remain -= len;
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

void ts::ERT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the sections
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    int section_number = 0;

    // Add fixed part (3 bytes). Will remain unmodified in all sections.
    PutUInt16(payload, information_provider_id);
    PutUInt8(payload + 2, uint8_t(relation_type << 4) | 0x0F);

    uint8_t* data = payload + 3;
    size_t remain = sizeof(payload) - 3;

    // Add all local events.
    for (auto it = relations.begin(); it != relations.end(); ++it) {

        // If we cannot at least add the fixed part of an event description, open a new section
        if (remain < 8) {
            addSection(table, section_number, payload, data, remain);
        }

        // If we are not at the beginning of the event loop, make sure that the entire
        // relation description fits in the section. If it does not fit, start a new section. Huge
        // descriptions may not fit into one section, even when starting at the beginning
        // of the description loop. In that case, the description will span two sections later.
        const Relation& rel(it->second);
        const DescriptorList& dlist(rel.descs);
        const size_t event_length = 8 + dlist.binarySize();
        if (data > payload + 3 && event_length > remain) {
            // Create a new section
            addSection(table, section_number, payload, data, remain);
        }

        // Fill fixed part of the content version.
        assert(remain >= 8);
        PutUInt16(data, rel.node_id);
        PutUInt8(data + 2, uint8_t(rel.collection_mode << 4) | 0x0F);
        PutUInt16(data + 3, rel.parent_node_id);
        PutUInt8(data + 5, rel.reference_number);
        data += 6; remain -= 6;

        // Serialize the descriptor loop.
        for (size_t start_index = 0; ; ) {

            // Insert descriptors (all or some).
            start_index = dlist.lengthSerialize(data, remain, start_index);

            // Exit loop when all descriptors were serialized.
            if (start_index >= dlist.count()) {
                break;
            }

            // Not all descriptors were written, the section is full.
            // Open a new one and continue with this relation description.
            addSection(table, section_number, payload, data, remain);
            PutUInt16(data, rel.node_id);
            PutUInt8(data + 2, uint8_t(rel.collection_mode << 4) | 0x0F);
            PutUInt16(data + 3, rel.parent_node_id);
            PutUInt8(data + 5, rel.reference_number);
            data += 6; remain -= 6;
        }
    }

    // Add partial section.
    addSection(table, section_number, payload, data, remain);
}


//----------------------------------------------------------------------------
// Add a new section to a table being serialized.
//----------------------------------------------------------------------------

void ts::ERT::addSection(BinaryTable& table, int& section_number, uint8_t* payload, uint8_t*& data, size_t& remain) const
{
    table.addSection(new Section(_table_id,
                                 true,                    // is_private_section
                                 event_relation_id,       // ts id extension
                                 version,
                                 is_current,
                                 uint8_t(section_number),
                                 uint8_t(section_number), //last_section_number
                                 payload,
                                 data - payload));        // payload_size,

    // Reinitialize pointers after fixed part of payload (3 bytes).
    remain += data - payload - 3;
    data = payload + 3;
    section_number++;
}


//----------------------------------------------------------------------------
// A static method to display a ERT section.
//----------------------------------------------------------------------------

void ts::ERT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    strm << margin << UString::Format(u"Event relation id: 0x%X (%d)", {section.tableIdExtension(), section.tableIdExtension()}) << std::endl;

    if (size >= 3) {

        // Display common information
        strm << margin << UString::Format(u"Information provider id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
             << margin << "Relation type: " << NameFromSection(u"ISDBRelationType", (data[2] >> 4) & 0x0F, names::DECIMAL_FIRST) << std::endl;
        data += 3; size -= 3;

        // Loop across all local events.
        while (size >= 8) {
            strm << margin << UString::Format(u"- Node id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
                 << margin << "  Collection mode: " << NameFromSection(u"ISDBCollectionMode", (data[2] >> 4) & 0x0F, names::DECIMAL_FIRST) << std::endl
                 << margin << UString::Format(u"  Parent node id: 0x%X (%d)", {GetUInt16(data + 3), GetUInt16(data + 3)}) << std::endl
                 << margin << UString::Format(u"  Reference number: 0x%X (%d)", {data[5], data[5]}) << std::endl;
            size_t len = GetUInt16(data + 6) & 0x0FFF;
            data += 8; size -= 8;
            len = std::min(len, size);
            display.displayDescriptorList(section, data, len, indent + 2);
            data += len; size -= len;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ERT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"event_relation_id", event_relation_id, true);
    root->setIntAttribute(u"information_provider_id", information_provider_id, true);
    root->setIntAttribute(u"relation_type", relation_type);

    for (auto it = relations.begin(); it != relations.end(); ++it) {
        xml::Element* e = root->addElement(u"relation");
        e->setIntAttribute(u"node_id", it->second.node_id, true);
        e->setIntAttribute(u"collection_mode", it->second.collection_mode);
        e->setIntAttribute(u"parent_node_id", it->second.parent_node_id, true);
        e->setIntAttribute(u"reference_number", it->second.reference_number, true);
        it->second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ERT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xrel;
    bool ok =
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute<uint16_t>(event_relation_id, u"event_relation_id", true) &&
        element->getIntAttribute<uint16_t>(information_provider_id, u"information_provider_id", true) &&
        element->getIntAttribute<uint8_t>(relation_type, u"relation_type", true, 0, 0, 15) &&
        element->getChildren(xrel, u"relation");

    for (auto it = xrel.begin(); ok && it != xrel.end(); ++it) {
        Relation& rel(relations.newEntry());
        ok = (*it)->getIntAttribute<uint16_t>(rel.node_id, u"node_id", true) &&
             (*it)->getIntAttribute<uint8_t>(rel.collection_mode, u"collection_mode", true, 0, 0, 15) &&
             (*it)->getIntAttribute<uint16_t>(rel.parent_node_id, u"parent_node_id", true) &&
             (*it)->getIntAttribute<uint8_t>(rel.reference_number, u"reference_number", true) &&
             rel.descs.fromXML(duck, *it);
    }
    return ok;
}
