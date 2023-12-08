//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsUNT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"UNT"
#define MY_CLASS ts::UNT
#define MY_TID ts::TID_UNT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Description of a compatibility descriptor.
//----------------------------------------------------------------------------

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

void ts::UNT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    action_type = uint8_t(section.tableIdExtension() >> 8);
    OUI = buf.getUInt24();
    processing_order = buf.getUInt8();

    // Get common descriptor loop.
    buf.getDescriptorListWithLength(descs);

    // Get descriptions of sets of devices.
    while (buf.canRead()) {

        // Create a new entry in the list of devices.
        Devices& devs(devices.newEntry());

        // Get compatibilityDescriptor(), a list of compatibility descriptors.
        // There is a leading 16-bit length field for compatibilityDescriptor().
        buf.pushReadSizeFromLength(16);
        size_t descriptorCount = buf.getUInt16();

        // Get outer descriptor loop.
        while (buf.canRead() && descriptorCount-- > 0) {
            CompatibilityDescriptor cdesc;
            cdesc.descriptorType = buf.getUInt8();

            // Get current compatibility descriptor content, based on 8-bit length field.
            buf.pushReadSizeFromLength(8);

            cdesc.specifierType = buf.getUInt8();
            cdesc.specifierData = buf.getUInt24();
            cdesc.model = buf.getUInt16();
            cdesc.version = buf.getUInt16();
            buf.skipBits(8); // ignore subDescriptorCount, just read them all
            buf.getDescriptorList(cdesc.subDescriptors);

            // Close current compatibility descriptor.
            buf.popState();

            // Insert compatibilityDescriptor() entry.
            devs.compatibilityDescriptor.push_back(cdesc);
        }

        // Close compatibilityDescriptor() list of compatibility descriptors.
        buf.popState();

        // Open platform loop using 16-bit length field.
        buf.pushReadSizeFromLength(16);

        // Get platform descriptions.
        while (buf.canRead()) {
            Platform& platform(devs.platforms.newEntry());
            buf.getDescriptorListWithLength(platform.target_descs);
            buf.getDescriptorListWithLength(platform.operational_descs);
        }

        // Close platform loop.
        buf.popState();
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::UNT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Fixed part, to be repeated on all sections.
    buf.putUInt24(OUI);
    buf.putUInt8(processing_order);
    buf.pushState();

    // Insert top-level common descriptor loop (with leading length field).
    // Add new section when the descriptor list overflows.
    for (size_t start = 0;;) {
        start = buf.putPartialDescriptorListWithLength(descs, start);
        if (buf.error() || start >= descs.size()) {
            break;
        }
        else {
            addOneSection(table, buf);
        }
    }

    // Add all sets of devices. A set of devices must be serialize inside one unique section.
    // If we cannot serialize a set of devices in the current section, open a new section.
    bool retry = false;
    auto it = devices.begin();
    while (!buf.error() && it != devices.end()) {
        const Devices& devs(it->second);

        // Try to serialize the current set of device in the current section.
        // Keep current position in case we cannot completely serialize it.
        buf.pushState();

        // Start of compatibilityDescriptor(). It is a structure with a 16-bit length field.
        buf.pushWriteSequenceWithLeadingLength(16);
        buf.putUInt16(uint16_t(devs.compatibilityDescriptor.size()));

        // Serialize all entries in the compatibilityDescriptor().
        for (auto it2 = devs.compatibilityDescriptor.begin(); !buf.error() && it2 != devs.compatibilityDescriptor.end(); ++it2) {
            buf.putUInt8(it2->descriptorType);
            buf.pushWriteSequenceWithLeadingLength(8); // descriptorLength
            buf.putUInt8(it2->specifierType);
            buf.putUInt24(it2->specifierData);
            buf.putUInt16(it2->model);
            buf.putUInt16(it2->version);
            buf.putUInt8(uint8_t(it2->subDescriptors.count()));
            buf.putDescriptorList(it2->subDescriptors);
            buf.popState(); // update descriptorLength
        }

        // End of compatibilityDescriptor(). The 16-bit length field is updated now.
        buf.popState();

        // Start of platform_loop. It is a structure with a 16-bit length field.
        buf.pushWriteSequenceWithLeadingLength(16);

        // Serialize all platform descriptions.
        for (auto it2 = devs.platforms.begin(); !buf.error() && it2 != devs.platforms.end(); ++it2) {
            buf.putDescriptorListWithLength(it2->second.target_descs);
            buf.putDescriptorListWithLength(it2->second.operational_descs);
        }

        // End of platform_loop. The 16-bit length field is updated now.
        buf.popState();

        // Process end of set of devices.
        if (!buf.error()) {
            // Set of devices was successfully serialized. Move to next one.
            retry = false;
            buf.dropState(); // drop initially saved position.
            ++it;
        }
        else if (retry) {
            // This is already a retry on an empty section. Definitely too large, invalid table.
            return;
        }
        else {
            // Could not serialize in this section, try with an empty one.
            retry = true;
            buf.popState(); // return to previous state before current set of devices
            buf.clearError();
            addOneSection(table, buf);
            buf.putUInt16(0xF000); // empty common_descriptor_loop.
        }
    }
}


//----------------------------------------------------------------------------
// A static method to display a UNT section.
//----------------------------------------------------------------------------

void ts::UNT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    if (!buf.canReadBytes(4)) {
        buf.setUserError();
    }
    else {
        const uint32_t oui = buf.getUInt24();
        const uint8_t oui_hash = section.tableIdExtension() & 0xFF;
        const uint8_t comp_hash = uint8_t(oui >> 16) ^ uint8_t(oui >> 8) ^ uint8_t(oui);
        const UString oui_check(oui_hash == comp_hash ? u"valid" : UString::Format(u"invalid, should be 0x%X", {comp_hash}));
        disp << margin << "OUI: " << NameFromOUI(oui, NamesFlags::HEXA_FIRST) << std::endl;
        disp << margin << UString::Format(u"Action type: 0x%X", {uint8_t(section.tableIdExtension() >> 8)});
        disp << UString::Format(u", processing order: 0x%X", {buf.getUInt8()});
        disp << UString::Format(u", OUI hash: 0x%X (%s)", {oui_hash, oui_check}) << std::endl;
    }

    // Display common descriptor loop.
    disp.displayDescriptorListWithLength(section, buf, margin, u"Common descriptors:", u"None");

    if (!buf.error()) {
        disp << margin << "Sets of devices:" << std::endl;
        if (buf.endOfRead()) {
            disp << margin << "- None" << std::endl;
        }
    }

    // Loop on sets of devices.
    for (size_t dev_index = 0; buf.canRead(); ++dev_index) {
        disp << margin << "- Devices " << dev_index << ":" << std::endl;

        // Display list of compatibility descriptor.
        buf.pushReadSizeFromLength(16);
        const size_t compatibilityDescriptorLength = buf.remainingReadBytes();
        size_t descriptorCount = buf.getUInt16();
        disp << margin << UString::Format(u"  Compatibility descriptor: %d bytes, %d descriptors", {compatibilityDescriptorLength, descriptorCount}) << std::endl;

        // Display outer descriptor loop.
        for (size_t desc_index = 0; buf.canRead() && descriptorCount-- > 0 && buf.canReadBytes(11); ++desc_index) {
            disp << margin
                 << "  - Descriptor " << desc_index
                 << ", type " << DataName(MY_XML_NAME, u"CompatibilityDescriptorType", buf.getUInt8(), NamesFlags::HEXA_FIRST)
                 << std::endl;

            // Get current compatibility descriptor content, based on 8-bit length field.
            buf.pushReadSizeFromLength(8);

            disp << margin << UString::Format(u"    Specifier type: 0x%X", {buf.getUInt8()});
            disp << UString::Format(u", specifier data (OUI): %s", {NameFromOUI(buf.getUInt24(), NamesFlags::HEXA_FIRST)}) << std::endl;
            disp << margin << UString::Format(u"    Model: 0x%X (%<d)", {buf.getUInt16()});
            disp << UString::Format(u", version: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            const size_t subDescriptorCount = buf.getUInt8();
            disp << margin << UString::Format(u"    Sub-descriptor count: %d", {subDescriptorCount}) << std::endl;

            // Display sub-descriptors. They are not real descriptors, so we display them in hexa.
            for (size_t subdesc_index = 0; buf.canRead() && subdesc_index < subDescriptorCount; ++subdesc_index) {
                disp << margin << UString::Format(u"    - Sub-descriptor %d, type: 0x%X (%<d)", {subdesc_index, buf.getUInt8()});
                size_t length = buf.getUInt8();
                disp << UString::Format(u", %d bytes", {length}) << std::endl;
                length = std::min(length, buf.remainingReadBytes());
                if (length > 0) {
                    disp << UString::Dump(buf.currentReadAddress(), length, UString::HEXA | UString::ASCII | UString::OFFSET, margin.size() + 6);
                }
                buf.skipBytes(length);
            }

            // Close current compatibility descriptor.
            disp.displayPrivateData(u"Extraneous data in compatibility descriptor", buf, NPOS, margin + u"    ");
            buf.popState();
        }

        // Close compatibilityDescriptor() list of compatibility descriptors.
        disp.displayPrivateData(u"Extraneous data in compatibility descriptors list", buf, NPOS, margin + u"  ");
        buf.popState();

        // Open platform loop using 16-bit length field.
        buf.pushReadSizeFromLength(16);

        // Get platform descriptions.
        for (size_t platform_index = 0; buf.canRead(); ++platform_index) {
            disp << margin << "  Platform " << platform_index << ":" << std::endl;
            disp.displayDescriptorListWithLength(section, buf, margin + u"    ", u"Target descriptors:", u"None");
            disp.displayDescriptorListWithLength(section, buf, margin + u"    ", u"Operational descriptors:", u"None");
        }

        // Close platform loop.
        disp.displayPrivateData(u"Extraneous data in platform loop", buf, NPOS, margin + u"  ");
        buf.popState();
    }
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

    for (const auto& it1 : devices) {
        const Devices& devs(it1.second);
        xml::Element* e1 = root->addElement(u"devices");
        // Loop on compatibilityDescriptor() entries.
        for (const auto& it2 : devs.compatibilityDescriptor) {
            xml::Element* e2 = e1->addElement(u"compatibilityDescriptor");
            e2->setIntAttribute(u"descriptorType", it2.descriptorType, true);
            e2->setIntAttribute(u"specifierType", it2.specifierType, true);
            e2->setIntAttribute(u"specifierData", it2.specifierData, true);
            e2->setIntAttribute(u"model", it2.model, true);
            e2->setIntAttribute(u"version", it2.version, true);
            // Loop on subdescriptors
            for (size_t i3 = 0; i3 < it2.subDescriptors.count(); ++i3) {
                const DescriptorPtr& desc(it2.subDescriptors[i3]);
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
        for (const auto& it2 : devs.platforms) {
            xml::Element* e2 = e1->addElement(u"platform");
            if (!it2.second.target_descs.empty()) {
                it2.second.target_descs.toXML(duck, e2->addElement(u"target"));
            }
            if (!it2.second.operational_descs.empty()) {
                it2.second.operational_descs.toXML(duck, e2->addElement(u"operational"));
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
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute(action_type, u"action_type", false, 0x01) &&
        element->getIntAttribute(OUI, u"OUI", true, 0, 0x000000, 0xFFFFFF) &&
        element->getIntAttribute(processing_order, u"processing_order", false, 0x00) &&
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
            ok = xcomdesc[i2]->getIntAttribute(comdesc.descriptorType, u"descriptorType", true) &&
                 xcomdesc[i2]->getIntAttribute(comdesc.specifierType, u"specifierType", false, 0x01) &&
                 xcomdesc[i2]->getIntAttribute(comdesc.specifierData, u"specifierData", true, 0, 0, 0xFFFFFF) &&
                 xcomdesc[i2]->getIntAttribute(comdesc.model, u"model", false, 0) &&
                 xcomdesc[i2]->getIntAttribute(comdesc.version, u"version", false, 0) &&
                 xcomdesc[i2]->getChildren(xsubdesc, u"subDescriptor");
            for (size_t i3 = 0; ok && i3 < xsubdesc.size(); ++i3) {
                uint8_t type = 0;
                ByteBlock content;
                ok = xsubdesc[i3]->getIntAttribute(type, u"subDescriptorType", true) &&
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
