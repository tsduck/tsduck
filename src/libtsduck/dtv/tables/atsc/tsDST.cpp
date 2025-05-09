//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDST.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DST"
#define MY_CLASS ts::DST
#define MY_TID ts::TID_DST
#define MY_STD ts::Standards::ATSC

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DST::DST(uint8_t version_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, CURRENT),
    apps(this),
    descs(this)
{
}

ts::DST::DST(DuckContext& duck, const BinaryTable& table) :
    DST()
{
    deserialize(duck, table);
}

ts::DST::DST(const DST& other) :
    AbstractLongTable(other),
    table_id_extension(other.table_id_extension),
    sdf_protocol_version(other.sdf_protocol_version),
    apps(this, other.apps),
    descs(this, other.descs),
    service_private_data(other.service_private_data)
{
}

ts::DST::Application::Application(const AbstractTable* table) :
    EntryWithDescriptors(table),
    taps(table)
{
}

ts::DST::Tap::Tap(const AbstractTable* table) :
    EntryWithDescriptors(table)
{
}


//----------------------------------------------------------------------------
// Inherited public methods
//----------------------------------------------------------------------------

bool ts::DST::isCurrent() const
{
    return CURRENT;
}

void ts::DST::setCurrent(bool is_current)
{
    _is_current = CURRENT;
}

uint16_t ts::DST::tableIdExtension() const
{
    return table_id_extension;
}

ts::DescriptorList* ts::DST::topLevelDescriptorList()
{
    return &descs;
}

const ts::DescriptorList* ts::DST::topLevelDescriptorList() const
{
    return &descs;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::DST::clearContent()
{
    table_id_extension = 0xFFFF;
    sdf_protocol_version = 1;
    apps.clear();
    descs.clear();
    service_private_data.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DST::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    table_id_extension = section.tableIdExtension();
    sdf_protocol_version = buf.getUInt8();
    size_t application_count_in_section = buf.getUInt8();

    // Get applications description
    while (!buf.error() && application_count_in_section-- > 0) {
        Application& app(apps.newEntry());
        app.compatibility_descriptor.deserialize(buf);
        // Get application id.
        buf.pushReadSizeFromLength(16);
        if (buf.canRead()) {
            app.app_id_description = buf.getUInt16();
            buf.getBytes(app.app_id);
        }
        buf.popState();
        // Get taps.
        size_t tap_count = buf.getUInt8();
        while (!buf.error() && tap_count-- > 0) {
            Tap& tap(app.taps.newEntry());
            tap.protocol_encapsulation = buf.getUInt8();
            buf.getBits(tap.action_type, 7);
            tap.resource_location = buf.getBool();
            tap.tap.deserialize(buf);
            // Get tap descriptors.
            buf.getDescriptorListWithLength(tap.descs, 16);
        }
        // Get application descriptors.
        buf.getDescriptorListWithLength(app.descs, 16);
        buf.getBytes(app.app_data, buf.getUInt16());
    }

    // Get top-level descriptor list.
    buf.getDescriptorListWithLength(descs, 16);
    buf.getBytes(service_private_data, buf.getUInt16());
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DST::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Add fixed fields.
    buf.putUInt8(sdf_protocol_version);

    // Save position before application_count_in_section. Will be updated at each application.
    uint8_t application_count_in_section = 0;
    buf.pushState();
    buf.putUInt8(application_count_in_section);
    const size_t payload_min_size = buf.currentWriteByteOffset() + 4;

    // Loop on application descriptions.
    for (auto app_it = apps.begin(); !buf.error() && app_it != apps.end(); ++app_it) {
        const Application& app(app_it->second);

        // We don't know the total size of the serialized application description and we don't know if it will fit
        // in the current section. So, we serialize the complete application into one specific buffer first.
        // Then, we will know if we can copy it in the current section or if we must create a new one.
        PSIBuffer appbuf(buf.duck(), MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE - payload_min_size - 4);

        // Serialize the complete application description in appbuf.
        app.compatibility_descriptor.serialize(buf);
        buf.pushWriteSequenceWithLeadingLength(16);
        if (app.app_id_description.has_value()) {
            buf.putUInt16(app.app_id_description.value());
            buf.putBytes(app.app_id);
        }
        buf.popState();
        buf.putUInt8(uint8_t(app.taps.size()));
        for (const auto& tap : app.taps) {
            buf.putUInt8(tap.second.protocol_encapsulation);
            buf.putBits(tap.second.action_type, 7);
            buf.putBit(tap.second.resource_location);
            tap.second.tap.serialize(buf);
            buf.putDescriptorListWithLength(tap.second.descs, 0, NPOS, 16);
        }
        buf.putDescriptorListWithLength(app.descs, 0, NPOS, 16);
        buf.putUInt16(uint16_t(app.app_data.size()));
        buf.putBytes(app.app_data);
        const size_t app_size = appbuf.currentWriteByteOffset();

        // If we are not at the beginning of the application loop, make sure that the entire
        // application fits in the section. If it does not fit, start a new section.
        if (app_size + 4 > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
            // Finish the section.
            buf.putUInt16(0); // service_info_length
            buf.putUInt16(0); // service_private_data_length
            // Create a new section.
            addOneSection(table, buf);
            // We are at the position of application_count_in_section in the new section.
            buf.putUInt8(application_count_in_section = 0);
        }

        // Copy the serialized application definition.
        buf.putBytes(appbuf.currentReadAddress(), app_size);

        // Now increment the field application_count_in_section at saved position.
        buf.swapState();
        buf.pushState();
        buf.putUInt8(++application_count_in_section);
        buf.popState();
        buf.swapState();
    }

    // End of application descriptions. Remain: top level descriptor loop and service_private_data.
    size_t start = 0;
    for (;;) {
        // Reduce the write area to keep space for service_private_data_length.
        buf.pushWriteSize(buf.remainingWriteBytes() - 2);
        // Serialize as many descritors as possible.
        start = buf.putPartialDescriptorListWithLength(descs, start, NPOS, 16);
        buf.popState();
        // Exit when all descriptors are gone.
        if (start >= descs.size()) {
            break;
        }
        // Complete the section and creates a new one.
        buf.putUInt16(0); // service_private_data_length
        addOneSection(table, buf);
        // We are at the position of application_count_in_section in the new section.
        buf.putUInt8(0); // application_count_in_section
    }

    // Finally serialize service_private_data.
    if (2 + service_private_data.size() > buf.remainingWriteBytes()) {
        // Complete the section and creates a new one.
        buf.putUInt16(0); // service_private_data_length
        addOneSection(table, buf);
        // We are at the position of application_count_in_section in the new section.
        buf.putUInt8(0);  // application_count_in_section
        buf.putUInt16(0); // service_info_length
    }
    buf.putUInt16(uint16_t(service_private_data.size()));
    buf.putBytes(service_private_data);
}


//----------------------------------------------------------------------------
// A static method to display a DST section.
//----------------------------------------------------------------------------

void ts::DST::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Table id extension: %n", section.tableIdExtension()) << std::endl;

    // Payload initial fixed part.
    if (!buf.canReadBytes(2)) {
        return;
    }
    disp << margin << UString::Format(u"SDF protocol version: %d", buf.getUInt8());
    const size_t app_count = buf.getUInt8();
    disp << ", number of applications: " << app_count << std::endl;

    // Loop on all applications.
    for (size_t app_index = 0; app_index < app_count; ++app_index) {
        disp << margin << "- Application #" << app_index << std::endl;
        if (!DSMCCCompatibilityDescriptor::Display(disp, buf, margin + u"  ") || !buf.canReadBytes(2)) {
            return;
        }
        const size_t app_id_byte_length = buf.getUInt16();
        if (app_id_byte_length >= 2) {
            if (!buf.canReadBytes(app_id_byte_length)) {
                return;
            }
            disp << margin << UString::Format(u"  App id description: %n", buf.getUInt16()) << std::endl;
            disp.displayPrivateData(u"App id", buf, app_id_byte_length - 2, margin + u"  ");
        }
        if (!buf.canReadBytes(1)) {
            return;
        }

        // Loop on all taps.
        const size_t tap_count = buf.getUInt8();
        disp << margin << "  Number of taps: " << tap_count << std::endl;
        for (size_t tap_index = 0; buf.canReadBytes(2) && tap_index < tap_count; ++tap_index) {
            disp << margin << "  - Tap #" << tap_index << std::endl;
            disp << margin << "    Protocol encapsulation: " << DataName(MY_XML_NAME, u"protocol_encapsulation", buf.getUInt8(), NamesFlags::HEX_VALUE_NAME) << std::endl;
            disp << margin << "    Action type: " << DataName(MY_XML_NAME, u"action_type", buf.getBits<uint8_t>(7), NamesFlags::HEX_VALUE_NAME) << std::endl;
            disp << margin << "    Resource location: " << int(buf.getBit()) << std::endl;
            if (!DSMCCTap::Display(disp, buf, margin + u"    ")) {
                return;
            }
            // Tap level descriptor list.
            if (!buf.canReadBytes(2)) {
                return;
            }
            DescriptorContext context(disp.duck(), section.tableId(), section.definingStandards(disp.duck().standards()));
            disp.displayDescriptorListWithLength(section, context, true, buf, margin + u"    ", u"Tap descriptors", UString(), 16);
        }

        // Application level descriptor list.
        if (!buf.canReadBytes(2)) {
            return;
        }
        DescriptorContext context(disp.duck(), section.tableId(), section.definingStandards(disp.duck().standards()));
        disp.displayDescriptorListWithLength(section, context, true, buf, margin + u"  ", u"Application descriptors", UString(), 16);

        // Application data.
        if (!buf.canReadBytes(2)) {
            return;
        }
        disp.displayPrivateData(u"Application data", buf, buf.getUInt16(), margin + u"  ");
    }

    // Top level descriptor list.
    if (!buf.canReadBytes(2)) {
        return;
    }
    DescriptorContext context(disp.duck(), section.tableId(), section.definingStandards(disp.duck().standards()));
    disp.displayDescriptorListWithLength(section, context, true, buf, margin, u"Service descriptors", UString(), 16);

    // Service private data.
    if (!buf.canReadBytes(2)) {
        return;
    }
    disp.displayPrivateData(u"Service private data", buf, buf.getUInt16(), margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DST::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", _version);
    root->setIntAttribute(u"table_id_extension", table_id_extension, true);
    root->setIntAttribute(u"sdf_protocol_version", sdf_protocol_version);

    for (const auto& app : apps) {
        xml::Element* xapp = root->addElement(u"application");
        app.second.compatibility_descriptor.toXML(duck, xapp);
        if (app.second.app_id_description.has_value()) {
            xml::Element* xid = xapp->addElement(u"app_id");
            xid->setIntAttribute(u"description", app.second.app_id_description.value(), true);
            xid->addHexaText(app.second.app_id, true);
        }
        for (const auto& tap : app.second.taps) {
            xml::Element* xtap = xapp->addElement(u"tap");
            xtap->setIntAttribute(u"protocol_encapsulation", tap.second.protocol_encapsulation, true);
            xtap->setIntAttribute(u"action_type", tap.second.action_type, true);
            xtap->setBoolAttribute(u"resource_location", tap.second.resource_location);
            tap.second.tap.toXML(duck, xtap);
            tap.second.descs.toXML(duck, xtap);
        }
        app.second.descs.toXML(duck, xapp);
        xapp->addHexaTextChild(u"app_data", app.second.app_data, true);
    }

    descs.toXML(duck, root);
    root->addHexaTextChild(u"service_private_data", service_private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DST::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xapps, unused;
    bool ok = element->getIntAttribute(_version, u"version", false, 0, 0, 31) &&
              element->getIntAttribute(table_id_extension, u"table_id_extension", false, 0xFFFF) &&
              element->getIntAttribute(sdf_protocol_version, u"sdf_protocol_version", false, 1) &&
              element->getChildren(xapps, u"application") &&
              element->getHexaTextChild(service_private_data, u"service_private_data") &&
              descs.fromXML(duck, unused, element, u"application,service_private_data");

    for (auto xapp : xapps) {
        xml::ElementVector xtaps, xid;
        Application& app(apps.newEntry());
        static const UString OTHER_TAGS = UString(u"app_id,app_data,tap,") + DSMCCCompatibilityDescriptor::DEFAULT_XML_NAME;
        ok = app.compatibility_descriptor.fromXML(duck, xapp) &&
             xapp->getChildren(xid, u"app_id", 0, 1) &&
             xapp->getChildren(xtaps, u"tap") &&
             xapp->getHexaTextChild(app.app_data, u"app_data") &&
             app.descs.fromXML(duck, unused, xapp, OTHER_TAGS) &&
             ok;
        if (ok && !xid.empty()) {
            app.app_id_description = uint16_t(0); // uint16_t cast required by MSVC.
            ok = xid[0]->getIntAttribute(app.app_id_description.value(), u"description", true) &&
                 xid[0]->getHexaText(app.app_id);
        }
        for (auto xtap : xtaps) {
            Tap& tap(app.taps.newEntry());
            ok = xtap->getIntAttribute(tap.protocol_encapsulation, u"protocol_encapsulation", true) &&
                 xtap->getIntAttribute(tap.action_type, u"action_type", true, 0, 0, 0x7F) &&
                 xtap->getBoolAttribute(tap.resource_location, u"resource_location", true) &&
                 tap.tap.fromXML(duck, xtap) &&
                 tap.descs.fromXML(duck, unused, xtap, DSMCCTap::DEFAULT_XML_NAME) &&
                 ok;
        }
    }
    return ok;
}
