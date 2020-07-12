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

#include "tsUNT.h"
#include "tsNames.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"UNT"
#define MY_CLASS ts::UNT
#define MY_TID ts::TID_UNT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Description of a compatibility descriptor.
//----------------------------------------------------------------------------

ts::UNT::CompatibilityDescriptor::CompatibilityDescriptor() :
    descriptorType(0xFF),  // user-defined
    specifierType(0x01),   // IEEE OUI
    specifierData(0),
    model(0),
    version(0),
    subDescriptors(nullptr)  // not real descriptors
{
}

ts::UNT::CompatibilityDescriptor::CompatibilityDescriptor(const CompatibilityDescriptor& other) :
    descriptorType(other.descriptorType),
    specifierType(other.specifierType),
    specifierData(other.specifierData),
    model(other.model),
    version(other.version),
    subDescriptors(nullptr, other.subDescriptors)
{
}


//----------------------------------------------------------------------------
// Description of a platform.
//----------------------------------------------------------------------------

ts::UNT::Platform::Platform(const AbstractTable* table) :
    EntryBase(),
    target_descs(table),
    operational_descs(table)
{
}

ts::UNT::Platform::Platform(const AbstractTable* table, const Platform& other) :
    EntryBase(other),
    target_descs(table, other.target_descs),
    operational_descs(table, other.operational_descs)
{
}


//----------------------------------------------------------------------------
// Description of a set of devices.
//----------------------------------------------------------------------------

ts::UNT::Devices::Devices(const AbstractTable* table) :
    EntryBase(),
    compatibilityDescriptor(),
    platforms(table)
{
}

ts::UNT::Devices::Devices(const AbstractTable* table, const Devices& other) :
    EntryBase(other),
    compatibilityDescriptor(other.compatibilityDescriptor),
    platforms(table, other.platforms)
{
}


//----------------------------------------------------------------------------
// UNT constructors
//----------------------------------------------------------------------------

ts::UNT::UNT(uint8_t version_, bool is_current_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, is_current_),
    action_type(0),
    OUI(0),
    processing_order(0),
    descs(this),
    devices(this)
{
}

ts::UNT::UNT(const UNT& other) :
    AbstractLongTable(other),
    action_type(other.action_type),
    OUI(other.OUI),
    processing_order(other.processing_order),
    descs(this, other.descs),
    devices(this, other.devices)
{
}

ts::UNT::UNT(DuckContext& duck, const BinaryTable& table) :
    UNT()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::UNT::tableIdExtension() const
{
    // The table id extension is made of action_type and OUI hash.
    return uint16_t(uint16_t(action_type) << 8) |
           uint16_t(((OUI >> 16) & 0xFF) ^ ((OUI >> 8) & 0xFF) ^ (OUI & 0xFF));

}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::UNT::clearContent()
{
    action_type = 0;
    OUI = 0;
    processing_order = 0;
    descs.clear();
    devices.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::UNT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Clear table content
    action_type = 0;
    OUI = 0;
    processing_order = 0;
    descs.clear();
    devices.clear();

    // Loop on all sections.
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        action_type = uint8_t(sect.tableIdExtension() >> 8);

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();

        // Get fixed part. We do not check the OUI_hash.
        if (remain < 4) {
            return;
        }
        OUI = GetUInt24(data);
        processing_order = data[3];
        data += 4;
        remain -= 4;

        // Get common descriptor loop.
        if (!DeserializeDescriptorList(descs, data, remain)) {
            return;
        }

        // Get descriptions of sets of devices.
        while (remain > 0) {
            Devices& devs(devices.newEntry());
            if (!DeserializeDevices(devs, data, remain)) {
                return;
            }
        }
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Deserialize a descriptor list.
//----------------------------------------------------------------------------

bool ts::UNT::DeserializeDescriptorList(DescriptorList& dlist, const uint8_t*& data, size_t& remain)
{
    // Get descriptor loop length.
    if (remain < 2) {
        return false;
    }
    size_t dlength = GetUInt16(data) & 0x0FFF;
    data += 2;
    remain -= 2;

    // Get descriptor loop.
    if (remain < dlength) {
        return false;
    }
    else {
        dlist.add(data, dlength);
        data += dlength;
        remain -= dlength;
        return true;
    }
}


//----------------------------------------------------------------------------
// Deserialize a compatibility descriptor (a list of them).
//----------------------------------------------------------------------------

bool ts::UNT::DeserializeCompatibilityDescriptorList(CompatibilityDescriptorList& dlist, const uint8_t*& data, size_t& remain)
{
    // Get descriptor loop length and count.
    if (remain < 4) {
        return false;
    }
    const size_t compatibilityDescriptorLength = GetUInt16(data);
    size_t descriptorCount = GetUInt16(data + 2);
    const uint8_t* const data_end = data + 2 + compatibilityDescriptorLength;

    // Check structure size.
    if (remain < 2 + compatibilityDescriptorLength) {
        return false;
    }
    data += 4;
    remain -= 4;

    // Get outer descriptor loop.
    while (descriptorCount > 0) {

        // Deserialize fixed part.
        if (remain < 11) {
            return false;
        }
        CompatibilityDescriptor cdesc;
        cdesc.descriptorType = data[0];
        const size_t descriptorLength = data[1];
        cdesc.specifierType = data[2];
        cdesc.specifierData = GetUInt24(data + 3);
        cdesc.model = GetUInt16(data + 6);
        cdesc.version = GetUInt16(data + 8);
        const size_t subDescriptorCount = data[10];
        if (remain < 2 + descriptorLength) {
            return false;
        }

        // Deserialize sub-descriptors.
        assert(descriptorLength >= 9);
        cdesc.subDescriptors.add(data + 11, descriptorLength - 9);
        data += 2 + descriptorLength;
        remain -= 2 + descriptorLength;

        // Check that the expected number of descriptors were read.
        if (cdesc.subDescriptors.size() != subDescriptorCount) {
            return false;
        }

        // Next compatibilityDescriptor() entry.
        dlist.push_back(cdesc);
        descriptorCount--;
    }

    // Check that we reached the expected end of structure.
    return data == data_end && descriptorCount == 0;
}


//----------------------------------------------------------------------------
// Deserialize a set of devices.
//----------------------------------------------------------------------------

bool ts::UNT::DeserializeDevices(Devices& devs, const uint8_t*& data, size_t& remain)
{
    // Get compatibility descriptor.
    if (!DeserializeCompatibilityDescriptorList(devs.compatibilityDescriptor, data, remain) || remain < 2) {
        return false;
    }

    // Get platform loop length.
    const uint8_t* const data_end = data + 2 + GetUInt16(data);
    data += 2;
    remain -= 2;

    // Get platform descriptions.
    while (data < data_end) {
        Platform& platform(devs.platforms.newEntry());
        if (!DeserializeDescriptorList(platform.target_descs, data, remain) || !DeserializeDescriptorList(platform.operational_descs, data, remain)) {
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::UNT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the sections
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    int section_number = 0;
    uint8_t* data = payload;
    size_t remain = sizeof(payload);

    // Serialize fixed part (4 bytes, will remain identical in all sections).
    PutUInt24(data, OUI);
    data[3] = processing_order;
    data += 4;
    remain -= 4;

    // Add top-level platform_descriptor_loop.
    // If the descriptor list is too long to fit into one section, create new sections when necessary.
    for (size_t start_index = 0; ; ) {

        // Add the descriptor list (or part of it).
        start_index = descs.lengthSerialize(data, remain, start_index);

        // If all descriptors were serialized, exit loop
        if (start_index == descs.count()) {
            break;
        }
        assert(start_index < descs.count());

        // Need to close the section and open a new one.
        addSection(table, section_number, payload, data, remain);
    }

    // Add all sets of devices. A set of devices must be serialize inside one unique section.
    // If we cannot serialize a set of devices in the current section, open a new section.
    // If a complete section is not large enough to serialize a device, the
    // device description is truncated.
    for (auto it = devices.begin(); it != devices.end(); ++it) {

        // Keep current position in case we cannot completely serialize the current set of devices.
        uint8_t* const initial_data = data;
        const size_t initial_remain = remain;

        // Try to serialize the current set of device in the current section.
        if (!serializeDevices(it->second, data, remain) && initial_data > payload + 6) {
            // Could not serialize the set of devices and there was already something before it.
            // Restore initial data and close the section.
            data = initial_data;
            remain = initial_remain;
            addSection(table, section_number, payload, data, remain);

            // Reserve empty common_descriptor_loop.
            PutUInt16(data, 0xF000);
            data += 2; remain -= 2;

            // Retry the serialization of the device. Ignore failure (truncated).
            serializeDevices(it->second, data, remain);
        }
    }

    // Add partial section (if there is one)
    if (data > payload + 6 || table.sectionCount() == 0) {
        addSection(table, section_number, payload, data, remain);
    }
}


//----------------------------------------------------------------------------
// Private method: Serialize one set of devices.
//----------------------------------------------------------------------------

bool ts::UNT::serializeDevices(const Devices& devs, uint8_t*& data, size_t& remain) const
{
    // Check if the structure is truncated (not enough space).
    bool truncated = false;

    // Minimum required size: 6 byte (4 for an empty compatibilityDescriptor() and 2 for platform_loop_length).
    constexpr size_t min_size = 6;
    if (remain < min_size) {
        return false;
    }

    // Keep room for 2 additional bytes for platform_loop_length.
    remain -= 2;

    // Serialize the compatibilityDescriptor().
    // Remember starting point to update compatibilityDescriptorLength and descriptorCount.
    // Skip them, they will be updated later.
    uint8_t* comp_desc_base = data;
    uint16_t descriptorCount = 0;
    data += 4;
    remain -= 4;

    // Serialize all entries in the compatibilityDescriptor().
    for (auto it = devs.compatibilityDescriptor.begin(); !truncated && it != devs.compatibilityDescriptor.end(); ++it) {
        // Check that we have space for the fixed part.
        if (remain < 11) {
            truncated = true;
            break;
        }
        // Fill fixed part. Skip descriptorLength and subDescriptorCount for now.
        uint8_t* desc_base = data;
        PutUInt8(data, it->descriptorType);
        PutUInt8(data + 2, it->specifierType);
        PutUInt24(data + 3, it->specifierData);
        PutUInt16(data + 6, it->model);
        PutUInt16(data + 8, it->version);
        data += 11;
        remain -= 11;
        // Serialize subDescriptor().
        const size_t count = it->subDescriptors.serialize(data, remain);
        // Update descriptorLength and subDescriptorCount.
        PutUInt8(desc_base + 1, uint8_t(data - desc_base - 2));
        PutUInt8(desc_base + 10, uint8_t(count));
        // Check if all sub-descriptors have been serialized.
        if (count < it->subDescriptors.count()) {
            truncated = true;
        }
        descriptorCount++;
    }

    // Update compatibilityDescriptorLength and descriptorCount.
    PutUInt16(comp_desc_base, uint16_t(data - comp_desc_base - 2));
    PutUInt16(comp_desc_base + 2, descriptorCount);

    // End of compatibilityDescriptor().
    // Restore additional bytes for platform_loop_length.
    remain += 2;

    // Save address of platform_loop_length and skip it.
    uint8_t* const platform_base = data;
    data += 2;
    remain -= 2;

    // Serialize all platform descriptions.
    for (auto it = devs.platforms.begin(); !truncated && it != devs.platforms.end(); ++it) {
        // Check that we have space for the fixed parts.
        if (remain < 4) {
            truncated = true;
            break;
        }
        // Serialize target descriptor loop, then operational descriptor loop.
        remain -= 2;
        const size_t tg_count = it->second.target_descs.lengthSerialize(data, remain);
        remain += 2;
        const size_t op_count = it->second.operational_descs.lengthSerialize(data, remain);
        // Check if we could serialize them all.
        truncated = tg_count < it->second.target_descs.count() || op_count < it->second.operational_descs.count();
    }

    // Update platform_loop_length.
    PutUInt16(platform_base, uint8_t(data - platform_base - 2));

    return !truncated;
}


//----------------------------------------------------------------------------
// Private method: Add a new section to a table being serialized.
//----------------------------------------------------------------------------

void ts::UNT::addSection(BinaryTable& table,
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
// A static method to display a UNT section.
//----------------------------------------------------------------------------

void ts::UNT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    if (size >= 4) {

        // Fixed part
        const uint8_t action_type = section.tableIdExtension() >> 8;
        const uint8_t oui_hash = section.tableIdExtension() & 0xFF;
        const uint8_t comp_hash = data[0] ^ data[1] ^ data[2];
        const uint32_t oui = GetUInt24(data);
        const uint8_t processing_order = data[3];
        data += 4; size -= 4;

        strm << margin << "OUI: " << names::OUI(oui, names::HEXA_FIRST) << std::endl
             << margin
             << UString::Format(u"Action type: 0x%X, processing order: 0x%X, OUI hash: 0x%X (%s)",
                                {action_type, processing_order, oui_hash,
                                 oui_hash == comp_hash ? u"valid" : UString::Format(u"invalid, should be 0x%X", {comp_hash})})
             << std::endl
             << margin << "Common descriptors:" << std::endl;

        // Display common descriptor loop.
        if (DisplayDescriptorList(display, section, data, size, indent + 2)) {
            // Loop on sets of devices.
            strm << margin << "Sets of devices:" << std::endl;
            if (size == 0) {
                strm << margin << "  None" << std::endl;
            }
            else {
                size_t dev_index = 0;
                while (size > 0) {
                    strm << margin << "  - Devices " << dev_index++ << ":" << std::endl;
                    if (!DisplayDevices(display, section, data, size, indent + 4)) {
                        break;
                    }
                }
            }
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// Display a descriptor list.
//----------------------------------------------------------------------------

bool ts::UNT::DisplayDescriptorList(TablesDisplay& display, const Section& section, const uint8_t*& data, size_t& remain, int indent)
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
        data += dlength;
        remain -= dlength;
    }

    return true;
}


//----------------------------------------------------------------------------
// Display a compatibility descriptor.
//----------------------------------------------------------------------------

bool ts::UNT::DisplayCompatibilityDescriptorList(TablesDisplay& display, const Section& section, const uint8_t*& data, size_t& remain, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    // Need fixed part.
    if (remain < 4) {
        return false;
    }
    const size_t compatibilityDescriptorLength = GetUInt16(data);
    const size_t descriptorCount = GetUInt16(data + 2);
    const uint8_t* const data_end = data + 2 + compatibilityDescriptorLength;

    strm << margin << UString::Format(u"Compatibility descriptor: %d bytes, %d descriptors", {compatibilityDescriptorLength, descriptorCount}) << std::endl;

    if (remain < 2 + compatibilityDescriptorLength) {
        return false;
    }
    data += 4;
    remain -= 4;

    // Display outer descriptor loop.
    for (size_t desc_index = 0; desc_index < descriptorCount; ++desc_index) {

        // Deserialize fixed part.
        if (remain < 11) {
            return false;
        }
        const size_t descriptorLength = data[1];
        const size_t subDescriptorCount = data[10];
        strm << margin
             << "- Descriptor " << desc_index
             << ", type " << NameFromSection(u"CompatibilityDescriptorType", data[0], names::HEXA_FIRST)
             << std::endl
             << margin
             << UString::Format(u"  Specifier type: 0x%X, specifier data (OUI): %s", {data[2], names::OUI(GetUInt24(data + 3), names::HEXA_FIRST)})
             << std::endl
             << margin
             << UString::Format(u"  Model: 0x%X (%d), version: 0x%X (%d)", {GetUInt16(data + 6), GetUInt16(data + 6), GetUInt16(data + 8), GetUInt16(data + 8)})
             << std::endl
             << margin
             << UString::Format(u"  Sub-descriptor count: %d", {subDescriptorCount})
             << std::endl;

        if (remain < 2 + descriptorLength) {
            return false;
        }

        // Locate sub-descriptors.
        const uint8_t* subdesc = data + 11;
        size_t subdesc_remain = descriptorLength - 9;

        // Then jump over them.
        data += 2 + descriptorLength;
        remain -= 2 + descriptorLength;

        // Display sub-descriptors. They are not real descriptors, so we display them in hexa.
        for (size_t subdesc_index = 0; subdesc_remain >= 2 && subdesc_index < subDescriptorCount; ++subdesc_index) {
            strm << margin
                 << UString::Format(u"  - Sub-descriptor %d, type: 0x%X (%d), %d bytes", {subdesc_index, subdesc[0], subdesc[0], subdesc[1]})
                 << std::endl;
            const size_t size = std::min<size_t>(subdesc_remain - 2, subdesc[1]);
            if (size > 0) {
                strm << UString::Dump(subdesc + 2, size, UString::HEXA | UString::ASCII | UString::OFFSET, indent + 4);
            }
            subdesc += 2 + size;
            subdesc_remain -= 2 + size;
        }
    }

    // Check that we reached the expected end of structure.
    return data == data_end;
}


//----------------------------------------------------------------------------
// Display a set of devices.
//----------------------------------------------------------------------------

bool ts::UNT::DisplayDevices(TablesDisplay& display, const Section& section, const uint8_t*& data, size_t& remain, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    // Display compatibility descriptor.
    if (!DisplayCompatibilityDescriptorList(display, section, data, remain, indent) || remain < 2) {
        return false;
    }

    // Get platform loop length.
    const uint8_t* const data_end = data + 2 + GetUInt16(data);
    data += 2;
    remain -= 2;

    // Display platform descriptions.
    size_t platform_index = 0;
    while (data < data_end) {
        strm << margin << "Platform " << platform_index++ << ":" << std::endl
             << margin << "  Target descriptors:" << std::endl;
        if (!DisplayDescriptorList(display, section, data, remain, indent + 2)) {
            return false;
        }
        strm << margin << "  Operational descriptors:" << std::endl;
        if (!DisplayDescriptorList(display, section, data, remain, indent + 2)) {
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::UNT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"action_type", action_type, true);
    root->setIntAttribute(u"OUI", OUI, true);
    root->setIntAttribute(u"processing_order", processing_order, true);
    descs.toXML(duck, root);

    for (auto it1 = devices.begin(); it1 != devices.end(); ++it1) {
        const Devices& devs(it1->second);
        xml::Element* e1 = root->addElement(u"devices");
        // Loop on compatibilityDescriptor() entries.
        for (auto it2 = devs.compatibilityDescriptor.begin(); it2 != devs.compatibilityDescriptor.end(); ++it2) {
            xml::Element* e2 = e1->addElement(u"compatibilityDescriptor");
            e2->setIntAttribute(u"descriptorType", it2->descriptorType, true);
            e2->setIntAttribute(u"specifierType", it2->specifierType, true);
            e2->setIntAttribute(u"specifierData", it2->specifierData, true);
            e2->setIntAttribute(u"model", it2->model, true);
            e2->setIntAttribute(u"version", it2->version, true);
            // Loop on subdescriptors
            for (size_t i3 = 0; i3 < it2->subDescriptors.count(); ++i3) {
                const DescriptorPtr& desc(it2->subDescriptors[i3]);
                if (!desc.isNull() && desc->isValid()) {
                    xml::Element* e3 = e2->addElement(u"subDescriptor");
                    e3->setIntAttribute(u"subDescriptorType", desc->tag(), true);
                    if (desc->payloadSize() > 0) {
                        e3->addHexaText(desc->payload(), desc->payloadSize());
                    }
                }
            }
        }
        // Loop on platform descriptions.
        for (auto it2 = devs.platforms.begin(); it2 != devs.platforms.end(); ++it2) {
            xml::Element* e2 = e1->addElement(u"platform");
            if (!it2->second.target_descs.empty()) {
                it2->second.target_descs.toXML(duck, e2->addElement(u"target"));
            }
            if (!it2->second.operational_descs.empty()) {
                it2->second.operational_descs.toXML(duck, e2->addElement(u"operational"));
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::UNT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xdevices;
    bool ok =
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute<uint8_t>(action_type, u"action_type", false, 0x01) &&
        element->getIntAttribute<uint32_t>(OUI, u"OUI", true, 0, 0x000000, 0xFFFFFF) &&
        element->getIntAttribute<uint8_t>(processing_order, u"processing_order", false, 0x00) &&
        descs.fromXML(duck, xdevices, element, u"devices");

    for (size_t i1 = 0; ok && i1 < xdevices.size(); ++i1) {
        Devices& devs(devices.newEntry());
        xml::ElementVector xcomdesc;
        xml::ElementVector xplatforms;
        ok = xdevices[i1]->getChildren(xcomdesc, u"compatibilityDescriptor") &&
             xdevices[i1]->getChildren(xplatforms, u"platform");

        for (size_t i2 = 0; ok && i2 < xcomdesc.size(); ++i2) {
            CompatibilityDescriptor comdesc;
            xml::ElementVector xsubdesc;
            ok = xcomdesc[i2]->getIntAttribute<uint8_t>(comdesc.descriptorType, u"descriptorType", true) &&
                 xcomdesc[i2]->getIntAttribute<uint8_t>(comdesc.specifierType, u"specifierType", false, 0x01) &&
                 xcomdesc[i2]->getIntAttribute<uint32_t>(comdesc.specifierData, u"specifierData", true, 0, 0, 0xFFFFFF) &&
                 xcomdesc[i2]->getIntAttribute<uint16_t>(comdesc.model, u"model", false, 0) &&
                 xcomdesc[i2]->getIntAttribute<uint16_t>(comdesc.version, u"version", false, 0) &&
                 xcomdesc[i2]->getChildren(xsubdesc, u"subDescriptor");
            for (size_t i3 = 0; ok && i3 < xsubdesc.size(); ++i3) {
                uint8_t type = 0;
                ByteBlock content;
                ok = xsubdesc[i3]->getIntAttribute<uint8_t>(type, u"subDescriptorType", true) &&
                     xsubdesc[i3]->getHexaText(content, 0, 255);
                if (ok) {
                    // Build complete descriptor.
                    content.insert(content.begin(), uint8_t(content.size()));
                    content.insert(content.begin(), type);
                    comdesc.subDescriptors.add(content.data(), content.size());
                }
            }
            devs.compatibilityDescriptor.push_back(comdesc);
        }

        for (size_t i2 = 0; ok && i2 < xplatforms.size(); ++i2) {
            Platform& platform(devs.platforms.newEntry());
            xml::ElementVector xtarget;
            xml::ElementVector xoperational;
            ok = xplatforms[i2]->getChildren(xtarget, u"target", 0, 1) &&
                 (xtarget.empty() || platform.target_descs.fromXML(duck, xtarget[0])) &&
                 xplatforms[i2]->getChildren(xoperational, u"operational", 0, 1) &&
                 (xoperational.empty() || platform.operational_descs.fromXML(duck, xoperational[0]));
        }
    }
    return ok;
}
