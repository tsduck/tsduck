//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSGT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"SGT"
#define MY_CLASS ts::SGT
#define MY_TID ts::TID_ASTRA_SGT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors and assignment.
//----------------------------------------------------------------------------

ts::SGT::SGT(uint8_t vers, bool cur, uint16_t id) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, cur),
    descs(this),
    services(this)
{
}

ts::SGT::SGT(DuckContext& duck, const BinaryTable& table) :
    SGT()
{
    deserialize(duck, table);
}

ts::SGT::SGT(const SGT& other) :
    AbstractLongTable(other),
    service_list_id(other.service_list_id),
    descs(this, other.descs),
    services(this, other.services)
{
}

ts::SGT::Service::Service(const AbstractTable* table) :
    EntryWithDescriptors(table)
{
}

void ts::SGT::clearContent()
{
    service_list_id = 0xFFFF;
    descs.clear();
    services.clear();
}


//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

uint16_t ts::SGT::tableIdExtension() const
{
    return service_list_id;
}

ts::DescriptorList* ts::SGT::topLevelDescriptorList()
{
    return &descs;
}

const ts::DescriptorList* ts::SGT::topLevelDescriptorList() const
{
    return &descs;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SGT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    service_list_id = section.tableIdExtension();

    // Get top-level descriptor list.
    buf.skipReservedBits(16);
    buf.getDescriptorListWithLength(descs);

    // Get service loop.
    buf.skipReservedBits(4);
    buf.pushReadSizeFromLength(12); // service_loop_length
    while (buf.canRead()) {
        ServiceIdTriplet id;
        id.service_id = buf.getUInt16();
        id.transport_stream_id = buf.getUInt16();
        id.original_network_id = buf.getUInt16();
        auto& serv(services[id]);
        buf.getBits(serv.logical_channel_number, 14);
        serv.visible_service_flag = buf.getBool();
        serv.new_service_flag = buf.getBool();
        serv.genre_code = buf.getUInt16();
        buf.getDescriptorListWithLength(serv.descs);
    }
    buf.popState(); // service_loop_length
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SGT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Minimum size of a section: 16-bit reserved, empty top-level descriptor list and service_loop_length.
    constexpr size_t payload_min_size = 6;

    // Add top-level descriptor list.
    // If the descriptor list is too long to fit into one section, create new sections when necessary.
    for (size_t start = 0;;) {
        buf.putReserved(16);
        // Reserve and restore 2 bytes for service_loop_length.
        buf.pushWriteSize(buf.remainingWriteBytes() - 2);
        start = buf.putPartialDescriptorListWithLength(descs, start);
        buf.popState();

        if (buf.error() || start >= descs.size()) {
            // Top-level descriptor list completed.
            break;
        }
        else {
            // There are remaining top-level descriptors, flush current section.
            // Add a zero service_loop_length.
            buf.putUInt16(0xF000);
            addOneSection(table, buf);
        }
    }

    // Reserve service_loop_length.
    buf.putReserved(4);
    buf.pushWriteSequenceWithLeadingLength(12);

    // Add all service entries.
    for (const auto& serv : services) {
        // Binary size of the service entry.
        const size_t entry_size = 12 + serv.second.descs.binarySize();

        // If the service description does not fit into the current section, start a new one.
        // Except if we are at the beginning of the section, in which case the service is too
        // large anyway for a section and will be truncated.
        if (entry_size > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
            // Update service_loop_length.
            buf.popState();

            // Add the section and reset buffer.
            addOneSection(table, buf);

            // Restart a new section with empty top-level descriptor list.
            buf.putReserved(16);
            buf.putUInt16(0xF000);
            buf.putReserved(4);
            buf.pushWriteSequenceWithLeadingLength(12);
        }

        // Serialize the service entry.
        buf.putUInt16(serv.first.service_id);
        buf.putUInt16(serv.first.transport_stream_id);
        buf.putUInt16(serv.first.original_network_id);
        buf.putBits(serv.second.logical_channel_number, 14);
        buf.putBit(serv.second.visible_service_flag);
        buf.putBit(serv.second.new_service_flag);
        buf.putUInt16(serv.second.genre_code);

        // Serialize the service descriptors.
        // Allow truncation if this is the only entry (too large) in a section.
        buf.putPartialDescriptorListWithLength(serv.second.descs);
    }

    // Update service_loop_length.
    buf.popState();
}


//----------------------------------------------------------------------------
// A static method to display a SGT section.
//----------------------------------------------------------------------------

void ts::SGT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Service list id: %n", section.tableIdExtension()) << std::endl;
    buf.skipReservedBits(16);

    DescriptorContext context(disp.duck(), section.tableId(), section.definingStandards(disp.duck().standards()));
    disp.displayDescriptorListWithLength(section, context, true, buf, margin, u"Service list information:");

    // Transport stream loop
    buf.skipReservedBits(4);
    buf.pushReadSizeFromLength(12); // service_loop_length
    while (buf.canReadBytes(6)) {
        disp << margin << UString::Format(u"- Service id: %n", buf.getUInt16());
        disp << UString::Format(u", TS id: %n", buf.getUInt16());
        disp << UString::Format(u", Network id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"  LCN: %d", buf.getBits<uint16_t>(14));
        disp << UString::Format(u", visible: %s", buf.getBool());
        disp << UString::Format(u", new: %s", buf.getBool());
        disp << UString::Format(u", genre code: %n", buf.getUInt16()) << std::endl;
        disp.displayDescriptorListWithLength(section, context, false, buf, margin + u"  ");
    }
    buf.popState(); // service_loop_length
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SGT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", _version);
    root->setBoolAttribute(u"current", _is_current);
    root->setIntAttribute(u"service_list_id", service_list_id, true);
    descs.toXML(duck, root);

    for (const auto& serv : services) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"service_id", serv.first.service_id, true);
        e->setIntAttribute(u"transport_stream_id", serv.first.transport_stream_id, true);
        e->setIntAttribute(u"original_network_id", serv.first.original_network_id, true);
        e->setIntAttribute(u"logical_channel_number", serv.second.logical_channel_number);
        e->setBoolAttribute(u"visible_service_flag", serv.second.visible_service_flag);
        e->setBoolAttribute(u"new_service_flag", serv.second.new_service_flag);
        e->setIntAttribute(u"genre_code", serv.second.genre_code, true);
        serv.second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SGT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getIntAttribute(_version, u"version", false, 0, 0, 31) &&
              element->getBoolAttribute(_is_current, u"current", false, true) &&
              element->getIntAttribute(service_list_id, u"service_list_id", true) &&
              descs.fromXML(duck, children, element, u"service");

    for (auto child : children) {
        ServiceIdTriplet id;
        ok = child->getIntAttribute(id.service_id, u"service_id", true) &&
             child->getIntAttribute(id.transport_stream_id, u"transport_stream_id", true) &&
             child->getIntAttribute(id.original_network_id, u"original_network_id", true) &&
             ok;
        if (ok) {
            auto& serv(services[id]);
            ok = child->getIntAttribute(serv.logical_channel_number, u"logical_channel_number", true, 0, 0, 0x3FFF) &&
                 child->getBoolAttribute(serv.visible_service_flag, u"visible_service_flag", false, true) &&
                 child->getBoolAttribute(serv.new_service_flag, u"new_service_flag", false, false) &&
                 child->getIntAttribute(serv.genre_code, u"genre_code", false, 0xFFFF) &&
                 serv.descs.fromXML(duck, child);
        }
    }
    return ok;
}
