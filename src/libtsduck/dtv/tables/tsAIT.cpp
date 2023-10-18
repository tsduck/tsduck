//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2018-2023, Tristan Claverie
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAIT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"AIT"
#define MY_CLASS ts::AIT
#define MY_TID ts::TID_AIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);


//----------------------------------------------------------------------------
// Constructors:
//----------------------------------------------------------------------------

ts::AIT::AIT(uint8_t version_, bool is_current_, uint16_t application_type_, bool test_application_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, is_current_),
    application_type(application_type_),
    test_application_flag(test_application_),
    descs(this),
    applications(this)
{
}

ts::AIT::AIT(DuckContext& duck, const BinaryTable& table) :
    AIT(0, true, 0, false)
{
    deserialize(duck, table);
}

ts::AIT::AIT(const AIT& other) :
    AbstractLongTable(other),
    application_type(other.application_type),
    test_application_flag(other.test_application_flag),
    descs(this, other.descs),
    applications(this, other.applications)
{
}

ts::AIT::Application::Application(const AbstractTable* table) :
    EntryWithDescriptors(table)
{
}



//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::AIT::tableIdExtension() const
{
    return (test_application_flag ? 0x8000 : 0x0000) | (application_type & 0x7FFF);
}


//----------------------------------------------------------------------------
// Get the maximum size in bytes of the payload of sections of this table.
//----------------------------------------------------------------------------

size_t ts::AIT::maxPayloadSize() const
{
    // Although a "private section" in the MPEG sense, the AIT section is limited to 1024 bytes in ETSI TS 101 812.
    return MAX_PSI_LONG_SECTION_PAYLOAD_SIZE;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::AIT::clearContent()
{
    application_type = 0;
    test_application_flag = false;
    descs.clear();
    applications.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AIT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    const uint16_t tid_ext = section.tableIdExtension();
    test_application_flag = (tid_ext & 0x8000) != 0;
    application_type = tid_ext & 0x7fff;

    // Get common descriptor list
    buf.getDescriptorListWithLength(descs);

    // Application loop length.
    buf.skipBits(4);
    const size_t loop_length = buf.getBits<size_t>(12);
    const size_t end_loop = buf.currentReadByteOffset() + loop_length;

    // Get application descriptions.
    while (buf.canRead()) {
        const uint32_t org_id = buf.getUInt32();
        const uint16_t app_id = buf.getUInt16();
        Application& app(applications[ApplicationIdentifier(org_id, app_id)]);
        app.control_code = buf.getUInt8();
        buf.getDescriptorListWithLength(app.descs);
    }

    // Make sure we exactly reached the end of transport stream loop.
    if (!buf.error() && buf.currentReadByteOffset() != end_loop) {
        buf.setUserError();
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AIT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Minimum size of a section: empty common descriptor list and application_loop_length.
    constexpr size_t payload_min_size = 4;

    // Add common descriptor list.
    // If the descriptor list is too long to fit into one section, create new sections when necessary.
    for (size_t start = 0;;) {
        // Reserve and restore 2 bytes for application_loop_length.
        buf.pushWriteSize(buf.size() - 2);
        start = buf.putPartialDescriptorListWithLength(descs, start);
        buf.popState();

        if (buf.error() || start >= descs.size()) {
            // Common descriptor list completed.
            break;
        }
        else {
            // There are remaining top-level descriptors, flush current section.
            // Add a zero application_loop_length.
            buf.putUInt16(0xF000);
            addOneSection(table, buf);
        }
    }

    // Reserve application_loop_length.
    buf.putBits(0xFF, 4);
    buf.pushWriteSequenceWithLeadingLength(12);

    // Add all transports
    for (const auto& it : applications) {

        // If we cannot at least add the fixed part of an application description, open a new section
        if (buf.remainingWriteBytes() < 9) {
            addSection(table, buf, false);
        }

        // Binary size of the application entry.
        const size_t entry_size = 9 + it.second.descs.binarySize();

        // If we are not at the beginning of the application loop, make sure that the entire
        // application description fits in the section. If it does not fit, start a new section.
        if (entry_size > buf.remainingWriteBytes() && buf.currentWriteByteOffset() > payload_min_size) {
            // Create a new section
            addSection(table, buf, false);
        }

        // Serialize the characteristics of the application.
        // If the descriptor list is too large for an entire section, it is truncated.
        buf.putUInt32(it.first.organization_id);
        buf.putUInt16(it.first.application_id);
        buf.putUInt8(it.second.control_code);
        buf.putPartialDescriptorListWithLength(it.second.descs);
    }

    // Add partial section.
    buf.popState(); // close application_loop_length
    addOneSection(table, buf);
}


//----------------------------------------------------------------------------
// Add a new section to a table being serialized, while in application loop.
//----------------------------------------------------------------------------

void ts::AIT::addSection(BinaryTable& table, PSIBuffer& payload, bool last_section) const
{
    // The read/write state was pushed just before application_loop_length.

    // Update application_loop_length.
    payload.popState();

    // Add the section and reset buffer.
    addOneSection(table, payload);

    // Prepare for the next section if necessary.
    if (!last_section) {
        // Empty (zero-length) top-level descriptor list.
        payload.putUInt16(0xF000);

        // Reserve application_loop_length.
        payload.putBits(0xFF, 4);
        payload.pushWriteSequenceWithLeadingLength(12);
    }
}


//----------------------------------------------------------------------------
// A static method to display a AIT section.
//----------------------------------------------------------------------------

void ts::AIT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    // Common information.
    const uint16_t tidext = section.tableIdExtension();
    disp << margin << UString::Format(u"Application type: %d (0x%<04X), Test application: %d", {tidext & 0x7FFF, tidext >> 15}) << std::endl;
    disp.displayDescriptorListWithLength(section, buf, margin, u"Common descriptor loop:");

    // Application loop
    buf.skipBits(4);
    buf.pushReadSizeFromLength(12);
    while (buf.canReadBytes(9)) {
        disp << margin << UString::Format(u"Application: Identifier: (Organization id: %d (0x%<X)", {buf.getUInt32()});
        disp << UString::Format(u", Application id: %d (0x%<X))", {buf.getUInt16()});
        disp << UString::Format(u", Control code: %d", {buf.getUInt8()}) << std::endl;
        disp.displayDescriptorListWithLength(section, buf, margin);
    }
    disp.displayPrivateData(u"Extraneous application data", buf, NPOS, margin);
    buf.popState();
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AIT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setBoolAttribute(u"test_application_flag", test_application_flag);
    root->setIntAttribute(u"application_type", application_type, true);
    descs.toXML(duck, root);

    for (const auto& app : applications) {
        xml::Element* e = root->addElement(u"application");
        e->setIntAttribute(u"control_code", app.second.control_code, true);
        xml::Element* id = e->addElement(u"application_identifier");
        id->setIntAttribute(u"organization_id", app.first.organization_id, true);
        id->setIntAttribute(u"application_id", app.first.application_id, true);
        app.second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AIT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getBoolAttribute(test_application_flag, u"test_application_flag", false, true) &&
        element->getIntAttribute(application_type, u"application_type", true, 0, 0x0000, 0x7FFF) &&
        descs.fromXML(duck, children, element, u"application");

    // Iterate through applications
    for (size_t index = 0; ok && index < children.size(); ++index) {
        Application application(this);
        ApplicationIdentifier identifier;
        const xml::Element* id = children[index]->findFirstChild(u"application_identifier", true);
        xml::ElementVector others;
        UStringList allowed({ u"application_identifier" });

        ok = children[index]->getIntAttribute(application.control_code, u"control_code", true, 0, 0x00, 0xFF) &&
             application.descs.fromXML(duck, others, children[index], allowed) &&
             id != nullptr &&
             id->getIntAttribute(identifier.organization_id, u"organization_id", true, 0, 0, 0xFFFFFFFF) &&
             id->getIntAttribute(identifier.application_id, u"application_id", true, 0, 0, 0xFFFF);

        if (ok) {
            applications[identifier] = application;
        }
    }
    return ok;
}
