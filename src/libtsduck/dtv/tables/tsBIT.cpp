//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsBIT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"BIT"
#define MY_CLASS ts::BIT
#define MY_TID ts::TID_BIT
#define MY_PID ts::PID_BIT
#define MY_STD ts::Standards::ISDB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::BIT::BIT(uint8_t vers, bool cur) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, cur),
    descs(this),
    broadcasters(this)
{
}

ts::BIT::BIT(const BIT& other) :
    AbstractLongTable(other),
    original_network_id(other.original_network_id),
    broadcast_view_propriety(other.broadcast_view_propriety),
    descs(this, other.descs),
    broadcasters(this, other.broadcasters)
{
}

ts::BIT::BIT(DuckContext& duck, const BinaryTable& table) :
    BIT()
{
    deserialize(duck, table);
}

ts::BIT::Broadcaster::Broadcaster(const AbstractTable* table) :
    EntryWithDescriptors(table)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::BIT::tableIdExtension() const
{
    return original_network_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::BIT::clearContent()
{
    original_network_id = 0;
    broadcast_view_propriety = false;
    descs.clear();
    broadcasters.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::BIT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    original_network_id = section.tableIdExtension();
    buf.skipBits(3);
    broadcast_view_propriety = buf.getBool();

    // Get top-level descriptor list.
    buf.getDescriptorListWithLength(descs);

    // Loop across all broadcasters
    while (buf.canRead()) {
        Broadcaster& bc(broadcasters[buf.getUInt8()]);
        buf.getDescriptorListWithLength(bc.descs);
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::BIT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Add top-level descriptor list at the beginning of the section.
    // The 4 bits before the descriptor loop length contain meaningful info.
    // If the descriptor list is too long to fit into one section, create new sections when necessary.
    for (size_t start_index = 0; ; ) {
        buf.putBits(0xFF, 3);
        buf.putBit(broadcast_view_propriety);
        start_index = buf.putPartialDescriptorListWithLength(descs, start_index);

        // If all descriptors were serialized, exit loop
        if (start_index == descs.count()) {
            break;
        }

        // Need to close the section and open a new one.
        addOneSection(table, buf);
    }

    // Minimal payload size, with an empty top-level descriptor list.
    constexpr size_t payload_min_size = 2;

    // Add all broadcasters.
    for (const auto& it : broadcasters) {
        const Broadcaster& bc(it.second);

        // Binary size of this broadcaster definition.
        const size_t entry_size = 3 + bc.descs.binarySize();

        // If we are not at the beginning of the broadcaster loop, make sure that the entire
        // entry fits in the section. If it does not fit, start a new section.
        if (entry_size > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
            // Create a new section.
            addOneSection(table, buf);
            // Insert an empty top-level descriptor list.
            buf.putUInt16(broadcast_view_propriety ? 0xF000 : 0xE000);
        }

        // Serialize the characteristics of the broadcaster. The section must be large enough to hold the entire descriptor list.
        buf.putUInt8(it.first);  // broadcaster_id
        buf.putDescriptorListWithLength(bc.descs);
    }
}


//----------------------------------------------------------------------------
// A static method to display a BIT section.
//----------------------------------------------------------------------------

void ts::BIT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Original network id: 0x%X (%<d)", {section.tableIdExtension()}) << std::endl;

    if (buf.canRead()) {
        buf.skipBits(3);
        disp << margin << UString::Format(u"Broadcast view property: %s", {buf.getBool()}) << std::endl;
        disp.displayDescriptorListWithLength(section, buf, margin, u"Common descriptors:");
    }

    // Loop across all broadcasters
    while (buf.canReadBytes(3)) {
        disp << margin << UString::Format(u"Broadcaster id: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        disp.displayDescriptorListWithLength(section, buf, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::BIT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"original_network_id", original_network_id, true);
    root->setBoolAttribute(u"broadcast_view_propriety", broadcast_view_propriety);
    descs.toXML(duck, root);

    for (const auto& it : broadcasters) {
        xml::Element* e = root->addElement(u"broadcaster");
        e->setIntAttribute(u"broadcaster_id", it.first, true);
        it.second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::BIT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xbroadcasters;
    bool ok =
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute(original_network_id, u"original_network_id", true) &&
        element->getBoolAttribute(broadcast_view_propriety, u"broadcast_view_propriety", true) &&
        descs.fromXML(duck, xbroadcasters, element, u"broadcaster");

    for (auto it = xbroadcasters.begin(); ok && it != xbroadcasters.end(); ++it) {
        uint8_t id;
        ok = (*it)->getIntAttribute(id, u"broadcaster_id", true) &&
             broadcasters[id].descs.fromXML(duck, *it);
    }
    return ok;
}
