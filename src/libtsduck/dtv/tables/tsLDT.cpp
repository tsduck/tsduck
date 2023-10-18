//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsLDT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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

void ts::LDT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    original_service_id = section.tableIdExtension();
    transport_stream_id = buf.getUInt16();
    original_network_id = buf.getUInt16();

    // Loop across all descriptions
    while (buf.canRead()) {
        Description& des(descriptions[buf.getUInt16()]);
        buf.skipBits(12);
        buf.getDescriptorListWithLength(des.descs);
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::LDT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Add fixed fields.
    buf.putUInt16(transport_stream_id);
    buf.putUInt16(original_network_id);
    buf.pushState();

    // Minimum payload size, before loop of descriptions.
    const size_t payload_min_size = buf.currentWriteByteOffset();

    // Add all descriptions.
    for (const auto& it : descriptions) {
        const Description& des(it.second);

        // Binary size of this entry.
        const size_t entry_size = 4 + des.descs.binarySize();

        // If we are not at the beginning of the content loop, make sure that the entire
        // entry fits in the section. If it does not fit, start a new section.
        if (entry_size > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
            // Create a new section.
            addOneSection(table, buf);
        }

        // Serialize the characteristics of the description. When the section is not large enough to hold
        // the entire descriptor list, open a new section for the rest of the descriptors. In that case,
        // the common properties of the description must be repeated.
        for (size_t start_index = 0; ; ) {
            buf.putUInt16(it.first); // description_id
            buf.putBits(0xFFFF, 12);
            start_index = buf.putPartialDescriptorListWithLength(des.descs, start_index);

            // Exit loop when all descriptors were serialized.
            if (start_index >= des.descs.count()) {
                break;
            }

            // Not all descriptors were written, the section is full.
            // Open a new one and continue with this entry.
            addOneSection(table, buf);
        }
    }
}


//----------------------------------------------------------------------------
// A static method to display a LDT section.
//----------------------------------------------------------------------------

void ts::LDT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Original service id: 0x%X (%<d)", {section.tableIdExtension()}) << std::endl;

    if (buf.canReadBytes(4)) {
        disp << margin << UString::Format(u"Transport stream id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Original network id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        while (buf.canReadBytes(5)) {
            disp << margin << UString::Format(u"Description id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            buf.skipBits(12);
            disp.displayDescriptorListWithLength(section, buf, margin);
        }
    }
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

    for (const auto& it : descriptions) {
        xml::Element* e = root->addElement(u"description");
        e->setIntAttribute(u"description_id", it.first, true);
        it.second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::LDT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xdescriptions;
    bool ok =
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute(original_service_id, u"original_service_id", true) &&
        element->getIntAttribute(transport_stream_id, u"transport_stream_id", true) &&
        element->getIntAttribute(original_network_id, u"original_network_id", true) &&
        element->getChildren(xdescriptions, u"description");

    for (auto it = xdescriptions.begin(); ok && it != xdescriptions.end(); ++it) {
        uint16_t id;
        ok = (*it)->getIntAttribute(id, u"description_id", true) &&
             descriptions[id].descs.fromXML(duck, *it);
    }
    return ok;
}
