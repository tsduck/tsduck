//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsINT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"INT"
#define MY_CLASS ts::INT
#define MY_TID ts::TID_INT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Description of a device.
//----------------------------------------------------------------------------

ts::INT::Device::Device(const AbstractTable* table) :
    target_descs(table),
    operational_descs(table)
{
}

ts::INT::Device::Device(const AbstractTable* table, const Device& other) :
    target_descs(table, other.target_descs),
    operational_descs(table, other.operational_descs)
{
}

ts::INT::Device& ts::INT::Device::operator=(const Device& other)
{
    if (&other != this) {
        // Copying the descriptor lists preserves the associated table of the target.
        target_descs = other.target_descs;
        operational_descs = other.operational_descs;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::INT::INT(uint8_t version_, bool is_current_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, is_current_),
    platform_descs(this),
    devices(this)
{
}

ts::INT::INT(const INT& other) :
    AbstractLongTable(other),
    action_type(other.action_type),
    platform_id(other.platform_id),
    processing_order(other.processing_order),
    platform_descs(this, other.platform_descs),
    devices(this, other.devices)
{
}

ts::INT::INT(DuckContext& duck, const BinaryTable& table) :
    INT()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::INT::tableIdExtension() const
{
    // The table id extension is made of action_type and platform_id_hash.
    return uint16_t(uint16_t(action_type) << 8) |
           uint16_t(((platform_id >> 16) & 0xFF) ^ ((platform_id >> 8) & 0xFF) ^ (platform_id& 0xFF));
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::INT::clearContent()
{
    action_type = 0;
    platform_id = 0;
    processing_order = 0;
    platform_descs.clear();
    devices.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::INT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    action_type = uint8_t(section.tableIdExtension() >> 8);
    platform_id = buf.getUInt24();
    processing_order = buf.getUInt8();

    // Get platform descriptor loop.
    buf.getDescriptorListWithLength(platform_descs);

    // Get device descriptions.
    while (buf.canRead()) {
        Device& dev(devices.newEntry());
        buf.getDescriptorListWithLength(dev.target_descs);
        buf.getDescriptorListWithLength(dev.operational_descs);
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::INT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Fixed part, to be repeated on all sections.
    buf.putUInt24(platform_id);
    buf.putUInt8(processing_order);
    buf.pushState();

    // Add top-level platform_descriptor_loop.
    // If the descriptor list is too long to fit into one section, create new sections when necessary.
    for (size_t start_index = 0; ; ) {

        // Add the descriptor list (or part of it).
        start_index = buf.putPartialDescriptorListWithLength(platform_descs, start_index);

        // If all descriptors were serialized, exit loop
        if (start_index >= platform_descs.size()) {
            break;
        }

        // Need to close the section and open a new one.
        addOneSection(table, buf);
    }

    // Minimum size of a section: fixed part and empty top-level descriptor list.
    constexpr size_t payload_min_size = 6;

    // Add all devices. A device must be serialize inside one unique section.
    // If we cannot serialize a device in the current section, open a new section.
    // If a complete section is not large enough to serialize a device, the
    // device description is truncated.
    for (auto& dev : devices) {

        // Binary size of the device entry.
        const size_t entry_size = 2 + dev.second.target_descs.binarySize() + 2 + dev.second.operational_descs.binarySize();

        // If the current entry does not fit into the section, create a new section, unless we are at the beginning of the section.
        if (entry_size > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
            addOneSection(table, buf);
            buf.putPartialDescriptorListWithLength(platform_descs, 0, 0);
        }

        // Insert device entry.
        // During serialization of the first descriptor loop, keep size for at least an empty second descriptor loop.
        buf.pushWriteSize(buf.size() - 2);
        buf.putPartialDescriptorListWithLength(dev.second.target_descs);
        buf.popState();
        buf.putPartialDescriptorListWithLength(dev.second.operational_descs);
    }
}


//----------------------------------------------------------------------------
// A static method to display a INT section.
//----------------------------------------------------------------------------

void ts::INT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    if (buf.canReadBytes(4)) {
        const uint8_t action = uint8_t(section.tableIdExtension() >> 8);
        const uint8_t id_hash = uint8_t(section.tableIdExtension());
        const uint32_t pfid = buf.getUInt24();
        const uint8_t order = buf.getUInt8();
        const uint8_t comp_hash = uint8_t(pfid >> 16) ^ uint8_t(pfid >> 8) ^ uint8_t(pfid);
        const UString hash_status(id_hash == comp_hash ? u"valid" : UString::Format(u"invalid, should be 0x%X", {comp_hash}));

        disp << margin << "Platform id: " << DataName(MY_XML_NAME, u"platform_id", pfid, NamesFlags::FIRST) << std::endl
             << margin
             << UString::Format(u"Action type: 0x%X, processing order: 0x%X, id hash: 0x%X (%s)", {action, order, id_hash, hash_status})
             << std::endl;

        disp.displayDescriptorListWithLength(section, buf, margin, u"Platform descriptors:");

        // Get device descriptions.
        for (int device_index = 0; buf.canRead(); device_index++) {
            disp << margin << "Device #" << device_index << std::endl;
            disp.displayDescriptorListWithLength(section, buf, margin + u"  ", u"Target descriptors:", u"None");
            disp.displayDescriptorListWithLength(section, buf, margin + u"  ", u"Operational descriptors:", u"None");
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::INT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"action_type", action_type, true);
    root->setIntAttribute(u"processing_order", processing_order, true);
    root->setIntAttribute(u"platform_id", platform_id, true);
    platform_descs.toXML(duck, root);

    for (auto& dev : devices) {
        if (!dev.second.target_descs.empty() || !dev.second.operational_descs.empty()) {
            xml::Element* e = root->addElement(u"device");
            if (!dev.second.target_descs.empty()) {
                dev.second.target_descs.toXML(duck, e->addElement(u"target"));
            }
            if (!dev.second.operational_descs.empty()) {
                dev.second.operational_descs.toXML(duck, e->addElement(u"operational"));
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::INT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute(action_type, u"action_type", false, 0x01) &&
        element->getIntAttribute(processing_order, u"processing_order", false, 0x00) &&
        element->getIntAttribute(platform_id, u"platform_id", true, 0, 0x000000, 0xFFFFFF) &&
        platform_descs.fromXML(duck, children, element, u"device");

    for (size_t index = 0; ok && index < children.size(); ++index) {
        Device& dev(devices.newEntry());
        xml::ElementVector target;
        xml::ElementVector operational;
        ok = children[index]->getChildren(target, u"target", 0, 1) &&
             (target.empty() || dev.target_descs.fromXML(duck, target[0])) &&
             children[index]->getChildren(operational, u"operational", 0, 1) &&
             (operational.empty() || dev.operational_descs.fromXML(duck, operational[0]));
    }
    return ok;
}
