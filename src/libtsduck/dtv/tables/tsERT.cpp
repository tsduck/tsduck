//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsERT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    EntryWithDescriptors(table)
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

void ts::ERT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    event_relation_id = section.tableIdExtension();
    information_provider_id = buf.getUInt16();
    buf.getBits(relation_type, 4);
    buf.skipBits(4);

    // Loop across all relations.
    while (buf.canRead()) {
        Relation& rel(relations.newEntry());
        rel.node_id = buf.getUInt16();
        buf.getBits(rel.collection_mode, 4);
        buf.skipBits(4);
        rel.parent_node_id = buf.getUInt16();
        rel.reference_number = buf.getUInt8();
        buf.getDescriptorListWithLength(rel.descs);
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ERT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Fixed part, to be repeated on all sections.
    buf.putUInt16(information_provider_id);
    buf.putBits(relation_type, 4);
    buf.putBits(0xFF, 4);
    buf.pushState();

    // Minimum payload size, before loop of local events.
    const size_t payload_min_size = buf.currentWriteByteOffset();

    // Add all relations.
    for (const auto& it : relations) {
        const Relation& rel(it.second);

        // Binary size of this entry.
        const size_t entry_size = 8 + rel.descs.binarySize();

        // If we are not at the beginning of the relations loop, make sure that the entire
        // description fits in the section. If it does not fit, start a new section.
        if (entry_size > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
            // Create a new section.
            addOneSection(table, buf);
        }

        // Serialize the relation entry. If the descriptor loop is too long, it is truncated.
        buf.putUInt16(rel.node_id);
        buf.putBits(rel.collection_mode, 4);
        buf.putBits(0xFF, 4);
        buf.putUInt16(rel.parent_node_id);
        buf.putUInt8(rel.reference_number);
        buf.putPartialDescriptorListWithLength(rel.descs);
    }
}


//----------------------------------------------------------------------------
// A static method to display a ERT section.
//----------------------------------------------------------------------------

void ts::ERT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Event relation id: 0x%X (%<d)", {section.tableIdExtension()}) << std::endl;

    if (buf.canReadBytes(3)) {
        disp << margin << UString::Format(u"Information provider id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << "Relation type: " << DataName(MY_XML_NAME, u"RelationType", buf.getBits<uint8_t>(4), NamesFlags::DECIMAL_FIRST) << std::endl;
        buf.skipBits(4);
        while (buf.canReadBytes(8)) {
            disp << margin << UString::Format(u"- Node id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp << margin << "  Collection mode: " << DataName(MY_XML_NAME, u"CollectionMode", buf.getBits<uint8_t>(4), NamesFlags::DECIMAL_FIRST) << std::endl;
            buf.skipBits(4);
            disp << margin << UString::Format(u"  Parent node id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp << margin << UString::Format(u"  Reference number: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
            disp.displayDescriptorListWithLength(section, buf, margin + u"  ");
        }
    }
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

    for (const auto& it : relations) {
        xml::Element* e = root->addElement(u"relation");
        e->setIntAttribute(u"node_id", it.second.node_id, true);
        e->setIntAttribute(u"collection_mode", it.second.collection_mode);
        e->setIntAttribute(u"parent_node_id", it.second.parent_node_id, true);
        e->setIntAttribute(u"reference_number", it.second.reference_number, true);
        it.second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ERT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xrel;
    bool ok =
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute(event_relation_id, u"event_relation_id", true) &&
        element->getIntAttribute(information_provider_id, u"information_provider_id", true) &&
        element->getIntAttribute(relation_type, u"relation_type", true, 0, 0, 15) &&
        element->getChildren(xrel, u"relation");

    for (auto it = xrel.begin(); ok && it != xrel.end(); ++it) {
        Relation& rel(relations.newEntry());
        ok = (*it)->getIntAttribute(rel.node_id, u"node_id", true) &&
             (*it)->getIntAttribute(rel.collection_mode, u"collection_mode", true, 0, 0, 15) &&
             (*it)->getIntAttribute(rel.parent_node_id, u"parent_node_id", true) &&
             (*it)->getIntAttribute(rel.reference_number, u"reference_number", true) &&
             rel.descs.fromXML(duck, *it);
    }
    return ok;
}
