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

#include "tsINT.h"
#include "tsNames.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    action_type(0),
    platform_id(0),
    processing_order(0),
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
// Deserialize a descriptor list. Update data and remain. Return true on success.
//----------------------------------------------------------------------------

bool ts::INT::GetDescriptorList(DescriptorList& dlist, const uint8_t*& data, size_t& remain)
{
    // Get descriptor loop length.
    if (remain < 2) {
        return false;
    }
    size_t dlength = GetUInt16(data) & 0x0FFF;
    data += 2;
    remain -= 2;

    if (remain < dlength) {
        return false;
    }

    // Get descriptor loop.
    dlist.add(data, dlength);
    data += dlength;
    remain -= dlength;
    return true;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::INT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Clear table content
    action_type = 0;
    platform_id = 0;
    processing_order = 0;
    platform_descs.clear();
    devices.clear();

    // Loop on all sections.
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        action_type = sect.tableIdExtension() >> 8;

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();

        // Get fixed part.
        if (remain < 4) {
            return;
        }
        platform_id = GetUInt24(data);
        processing_order = data[3];
        data += 4;
        remain -= 4;

        // Get platform descriptor loop.
        if (!GetDescriptorList(platform_descs, data, remain)) {
            return;
        }

        // Get device descriptions.
        while (remain > 0) {
            Device& dev(devices.newEntry());
            if (!GetDescriptorList(dev.target_descs, data, remain) || !GetDescriptorList(dev.operational_descs, data, remain)) {
                return;
            }
        }
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::INT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the sections
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    int section_number = 0;
    uint8_t* data = payload;
    size_t remain = sizeof(payload);

    // Serialize fixed part (4 bytes, will remain identical in all sections).
    PutUInt24(data, platform_id);
    data[3] = processing_order;
    data += 4;
    remain -= 4;

    // Add top-level platform_descriptor_loop.
    // If the descriptor list is too long to fit into one section, create new sections when necessary.
    for (size_t start_index = 0; ; ) {

        // Add the descriptor list (or part of it).
        start_index = platform_descs.lengthSerialize(data, remain, start_index);

        // If all descriptors were serialized, exit loop
        if (start_index == platform_descs.count()) {
            break;
        }
        assert(start_index < platform_descs.count());

        // Need to close the section and open a new one.
        addSection(table, section_number, payload, data, remain);
    }

    // Add all devices. A device must be serialize inside one unique section.
    // If we cannot serialize a device in the current section, open a new section.
    // If a complete section is not large enough to serialize a device, the
    // device description is truncated.
    for (DeviceList::const_iterator it = devices.begin(); it != devices.end(); ++it) {

        // Keep current position in case we cannot completely serialize the current device.
        uint8_t* const initial_data = data;
        const size_t initial_remain = remain;

        // Try to serialize the current device in the current section.
        if (!serializeDevice(it->second, data, remain) && initial_data > payload + 6) {
            // Could not serialize the device and there was already something before it.
            // Restore initial data and close the section.
            data = initial_data;
            remain = initial_remain;
            addSection(table, section_number, payload, data, remain);

            // Reserve empty platform_descriptor_loop.
            PutUInt16(data, 0xF000);
            data += 2; remain -= 2;

            // Retry the serialization of the device. Ignore failure (truncated).
            serializeDevice(it->second, data, remain);
        }
    }

    // Add partial section (if there is one)
    if (data > payload + 6 || table.sectionCount() == 0) {
        addSection(table, section_number, payload, data, remain);
    }
}


//----------------------------------------------------------------------------
// Private method: Serialize one device description. Update data and remain.
// Return true if the service was completely serialized, false otherwise.
//----------------------------------------------------------------------------

bool ts::INT::serializeDevice(const Device& device, uint8_t*& data, size_t& remain) const
{
    // We need at least 4 bytes, for the length of the two descriptor loops.
    if (remain < 4) {
        return false;
    }

    // Keep 2 additional bytes for operational descriptor loop length.
    remain -= 2;

    // Serialize target descriptor loop, then operational descriptor loop.
    const size_t tg_count = device.target_descs.lengthSerialize(data, remain);
    remain += 2;
    const size_t op_count = device.operational_descs.lengthSerialize(data, remain);

    // Return if we could serialize them all.
    return tg_count == device.target_descs.count() && op_count == device.operational_descs.count();
}


//----------------------------------------------------------------------------
// Private method: Add a new section to a table being serialized.
// Section number is incremented. Data and remain are reinitialized.
//----------------------------------------------------------------------------

void ts::INT::addSection(BinaryTable& table,
                         int& section_number,
                         uint8_t* payload,
                         uint8_t*& data,
                         size_t& remain) const
{
    table.addSection(new Section(_table_id,
                                 true,    // is_private_section
                                 tableIdExtension(),
                                 version,
                                 is_current,
                                 uint8_t(section_number),
                                 uint8_t(section_number),   //last_section_number
                                 payload,
                                 data - payload)); // payload_size,

    // Reinitialize pointers.
    // Restart after constant part of payload (4 bytes).
    remain += data - payload - 4;
    data = payload + 4;
    section_number++;
}


//----------------------------------------------------------------------------
// Display a descriptor list. Update data and remain. Return true on success.
//----------------------------------------------------------------------------

bool ts::INT::DisplayDescriptorList(TablesDisplay& display, const Section& section, const uint8_t*& data, size_t& remain, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (remain < 2) {
        return false;
    }

    size_t dlength = GetUInt16(data) & 0x0FFF;
    data += 2;
    remain -= 2;

    if (remain < dlength) {
        return false;
    }

    if (dlength == 0) {
        strm << margin << "None" << std::endl;
    }
    else {
        display.displayDescriptorList(section, data, dlength, indent);
        data += dlength; remain -= dlength;
    }

    return true;
}


//----------------------------------------------------------------------------
// A static method to display a INT section.
//----------------------------------------------------------------------------

void ts::INT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    if (size >= 4) {

        // Fixed part
        const uint8_t action_type = section.tableIdExtension() >> 8;
        const uint8_t id_hash = section.tableIdExtension() & 0xFF;
        const uint8_t comp_hash = data[0] ^ data[1] ^ data[2];
        const uint32_t platform_id = GetUInt24(data);
        const uint8_t processing_order = data[3];
        data += 4; size -= 4;

        strm << margin << "Platform id: " << names::PlatformId(platform_id, names::FIRST) << std::endl
             << margin
             << UString::Format(u"Action type: 0x%X, processing order: 0x%X, id hash: 0x%X (%s)",
                                {action_type, processing_order, id_hash,
                                 id_hash == comp_hash ? u"valid" : UString::Format(u"invalid, should be 0x%X", {comp_hash})})
             << std::endl
             << margin << "Platform descriptors:" << std::endl;

        // Get platform descriptor loop.
        if (DisplayDescriptorList(display, section, data, size, indent + 2)) {
            // Get device descriptions.
            int device_index = 0;
            bool ok = true;
            while (ok && size > 0) {
                strm << margin << "Device #" << device_index++ << std::endl
                     << margin << "  Target descriptors:" << std::endl;
                ok = DisplayDescriptorList(display, section, data, size, indent + 4);
                if (ok) {
                    strm << margin << "  Operational descriptors:" << std::endl;
                    ok = DisplayDescriptorList(display, section, data, size, indent + 4);
                }
            }
        }
    }

    display.displayExtraData(data, size, indent);
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

    for (DeviceList::const_iterator it = devices.begin(); it != devices.end(); ++it) {
        const Device& dev(it->second);
        if (!dev.target_descs.empty() || !dev.operational_descs.empty()) {
            xml::Element* e = root->addElement(u"device");
            if (!dev.target_descs.empty()) {
                dev.target_descs.toXML(duck, e->addElement(u"target"));
            }
            if (!dev.operational_descs.empty()) {
                dev.operational_descs.toXML(duck, e->addElement(u"operational"));
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
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute<uint8_t>(action_type, u"action_type", false, 0x01) &&
        element->getIntAttribute<uint8_t>(processing_order, u"processing_order", false, 0x00) &&
        element->getIntAttribute<uint32_t>(platform_id, u"platform_id", true, 0, 0x000000, 0xFFFFFF) &&
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
