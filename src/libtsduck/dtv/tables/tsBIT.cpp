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

#include "tsBIT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    original_network_id(0),
    broadcast_view_propriety(false),
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

void ts::BIT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Clear table content
    descs.clear();
    broadcasters.clear();

    // Loop on all sections
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Abort if not expected table
        if (sect.tableId() != _table_id) {
            return;
        }

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        original_network_id = sect.tableIdExtension();

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();

        // Get top-level descriptor list
        if (remain < 2) {
            return;
        }
        broadcast_view_propriety = (data[0] & 0x10) != 0;
        size_t length = GetUInt16(data) & 0x0FFF;
        data += 2; remain -= 2;

        length = std::min(length, remain);
        descs.add(data, length);
        data += length; remain -= length;

        // Loop across all broadcasters
        while (remain >= 3) {
            const uint8_t id = data[0];
            length = GetUInt16(data + 1) & 0x0FFF;
            data += 3; remain -= 3;

            length = std::min(length, remain);
            broadcasters[id].descs.add(data, length);
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

void ts::BIT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the sections
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    int section_number = 0;
    uint8_t* data = payload;
    size_t remain = sizeof(payload);

    // Add top-level descriptor list.
    // If the descriptor list is too long to fit into one section, create new sections when necessary.
    for (size_t start_index = 0; ; ) {

        // Add the descriptor list (or part of it).
        start_index = descs.lengthSerialize(data, remain, start_index, broadcast_view_propriety ? 0x000F : 0x000E);

        // If all descriptors were serialized, exit loop
        if (start_index == descs.count()) {
            break;
        }
        assert(start_index < descs.count());

        // Need to close the section and open a new one.
        addSection(table, section_number, payload, data, remain);
    }

    // Add all broadcasters.
    for (auto it = broadcasters.begin(); it != broadcasters.end(); ++it) {

        // If we cannot at least add the fixed part of a broadcaster, open a new section
        if (remain < 1) {
            addSection(table, section_number, payload, data, remain);
            createEmptyMainDescriptorLoop(data, remain);
        }

        // If we are not at the beginning of the broadcaster loop, make sure that the entire broadcaster
        // description fits in the section. If it does not fit, start a new section. Huge broadcaster
        // descriptions may not fit into one section, even when starting at the beginning of the broadcaster
        // loop. In that case, the broadcaster description will span two sections later.
        const DescriptorList& dlist(it->second.descs);
        if (data > payload + 2 && 1 + dlist.binarySize() > remain) {
            // Create a new section
            addSection(table, section_number, payload, data, remain);
            createEmptyMainDescriptorLoop(data, remain);
        }

        // Serialize the characteristics of the broadcaster. When the section is not large enough to hold
        // the entire descriptor list, open a new section for the rest of the descriptors. In that case,
        // the common properties of the broadcaster must be repeated.
        for (size_t start_index = 0; ; ) {
            // Insert common characteristics of the broadcaster (ie. broadcaster id).
            assert(remain >= 3);
            data[0] = it->first;
            data++; remain--;

            // Insert descriptors (all or some).
            start_index = dlist.lengthSerialize(data, remain, start_index);

            // Exit loop when all descriptors were serialized.
            if (start_index >= dlist.count()) {
                break;
            }

            // Not all descriptors were written, the section is full.
            // Open a new one and continue with this transport.
            addSection(table, section_number, payload, data, remain);
            createEmptyMainDescriptorLoop(data, remain);
        }
    }

    // Add partial section.
    addSection(table, section_number, payload, data, remain);
}


//----------------------------------------------------------------------------
// Create an empty main descriptor loop. Data and remain are updated.
//----------------------------------------------------------------------------

void ts::BIT::createEmptyMainDescriptorLoop(uint8_t*& data, size_t& remain) const
{
    assert(remain >= 2);
    data[0] = broadcast_view_propriety ? 0xF0 : 0xE0;
    data[1] = 0x00;
    data += 2; remain -= 2;
}


//----------------------------------------------------------------------------
// Add a new section to a table being serialized.
//----------------------------------------------------------------------------

void ts::BIT::addSection(BinaryTable& table, int& section_number, uint8_t* payload, uint8_t*& data, size_t& remain) const
{
    table.addSection(new Section(_table_id,
                                 true,                    // is_private_section
                                 original_network_id,     // ts id extension
                                 version,
                                 is_current,
                                 uint8_t(section_number),
                                 uint8_t(section_number), //last_section_number
                                 payload,
                                 data - payload));        // payload_size,

    // Reinitialize pointers.
    remain += data - payload;
    data = payload;
    section_number++;
}


//----------------------------------------------------------------------------
// A static method to display a BIT section.
//----------------------------------------------------------------------------

void ts::BIT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    strm << margin << UString::Format(u"Original network id: 0x%X (%d)", {section.tableIdExtension(), section.tableIdExtension()}) << std::endl;

    if (size >= 2) {
        // Display common information
        strm << margin << UString::Format(u"Broadcast view property: %s", {(data[0] & 0x10) != 0}) << std::endl;
        size_t length = GetUInt16(data) & 0x0FFF;
        data += 2; size -= 2;
        if (length > size) {
            length = size;
        }
        if (length > 0) {
            strm << margin << "Common descriptors:" << std::endl;
            display.displayDescriptorList(section, data, length, indent);
        }
        data += length; size -= length;

        // Loop across all broadcasters
        while (size >= 3) {
            strm << margin << UString::Format(u"Broadcaster id: 0x%X (%d)", {data[0], data[0]}) << std::endl;
            length = GetUInt16(data + 1) & 0x0FFF;
            data += 3; size -= 3;
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

void ts::BIT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"original_network_id", original_network_id, true);
    root->setBoolAttribute(u"broadcast_view_propriety", broadcast_view_propriety);
    descs.toXML(duck, root);

    for (auto it = broadcasters.begin(); it != broadcasters.end(); ++it) {
        xml::Element* e = root->addElement(u"broadcaster");
        e->setIntAttribute(u"broadcaster_id", it->first, true);
        it->second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::BIT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xbroadcasters;
    bool ok =
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute<uint16_t>(original_network_id, u"original_network_id", true) &&
        element->getBoolAttribute(broadcast_view_propriety, u"broadcast_view_propriety", true) &&
        descs.fromXML(duck, xbroadcasters, element, u"broadcaster");

    for (auto it = xbroadcasters.begin(); ok && it != xbroadcasters.end(); ++it) {
        uint8_t id;
        ok = (*it)->getIntAttribute<uint8_t>(id, u"broadcaster_id", true) &&
             broadcasters[id].descs.fromXML(duck, *it);
    }
    return ok;
}
