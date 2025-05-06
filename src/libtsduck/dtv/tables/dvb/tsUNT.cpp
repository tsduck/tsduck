//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
#include "tsOUI.h"

#define MY_XML_NAME u"UNT"
#define MY_CLASS ts::UNT
#define MY_TID ts::TID_UNT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Description of a platform.
//----------------------------------------------------------------------------

ts::UNT::Platform::Platform(const AbstractTable* table) :
    AttachedEntry(),
    target_descs(table),
    operational_descs(table)
{
}

ts::UNT::Platform::Platform(const AbstractTable* table, const Platform& other) :
    AttachedEntry(other),
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
    AttachedEntry(other),
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
// Inherited public methods
//----------------------------------------------------------------------------

uint16_t ts::UNT::tableIdExtension() const
{
    // The table id extension is made of action_type and OUI hash.
    return uint16_t(uint16_t(action_type) << 8) |
           uint16_t(((OUI >> 16) & 0xFF) ^ ((OUI >> 8) & 0xFF) ^ (OUI & 0xFF));
}

ts::DescriptorList* ts::UNT::topLevelDescriptorList()
{
    return &descs;
}

const ts::DescriptorList* ts::UNT::topLevelDescriptorList() const
{
    return &descs;
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

        // Get compatibilityDescriptor().
        devs.compatibilityDescriptor.deserialize(buf);

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
        devs.compatibilityDescriptor.serialize(buf);

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
        const UString oui_check(oui_hash == comp_hash ? u"valid" : UString::Format(u"invalid, should be 0x%X", comp_hash));
        disp << margin << "OUI: " << OUIName(oui, NamesFlags::HEX_VALUE_NAME) << std::endl;
        disp << margin << UString::Format(u"Action type: 0x%X", uint8_t(section.tableIdExtension() >> 8));
        disp << UString::Format(u", processing order: 0x%X", buf.getUInt8());
        disp << UString::Format(u", OUI hash: 0x%X (%s)", oui_hash, oui_check) << std::endl;
    }

    // Display common descriptor loop.
    DescriptorContext context(disp.duck(), section.tableId(), section.definingStandards(disp.duck().standards()));
    disp.displayDescriptorListWithLength(section, context, true, buf, margin, u"Common descriptors:", u"None");

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
        DSMCCCompatibilityDescriptor::Display(disp, buf, margin + u"  ");

        // Open platform loop using 16-bit length field.
        buf.pushReadSizeFromLength(16);

        // Get platform descriptions.
        for (size_t platform_index = 0; buf.canRead(); ++platform_index) {
            disp << margin << "  Platform " << platform_index << ":" << std::endl;
            disp.displayDescriptorListWithLength(section, context, false, buf, margin + u"    ", u"Target descriptors:", u"None");
            disp.displayDescriptorListWithLength(section, context, false, buf, margin + u"    ", u"Operational descriptors:", u"None");
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
    root->setIntAttribute(u"version", _version);
    root->setBoolAttribute(u"current", _is_current);
    root->setIntAttribute(u"action_type", action_type, true);
    root->setIntAttribute(u"OUI", OUI, true);
    root->setIntAttribute(u"processing_order", processing_order, true);
    descs.toXML(duck, root);

    for (const auto& it1 : devices) {
        const Devices& devs(it1.second);
        xml::Element* e1 = root->addElement(u"devices");
        devs.compatibilityDescriptor.toXML(duck, e1);
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
        element->getIntAttribute(_version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(_is_current, u"current", false, true) &&
        element->getIntAttribute(action_type, u"action_type", false, 0x01) &&
        element->getIntAttribute(OUI, u"OUI", true, 0, 0x000000, 0xFFFFFF) &&
        element->getIntAttribute(processing_order, u"processing_order", false, 0x00) &&
        descs.fromXML(duck, xdevices, element, u"devices");

    for (size_t i1 = 0; ok && i1 < xdevices.size(); ++i1) {
        Devices& devs(devices.newEntry());
        xml::ElementVector xplatforms;
        ok = devs.compatibilityDescriptor.fromXML(duck, xdevices[i1]) &&
             xdevices[i1]->getChildren(xplatforms, u"platform");

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
