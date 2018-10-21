//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2018, Tristan Claverie
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
//
//  Representation of an Application Information Table (AIT)
//
//----------------------------------------------------------------------------

#include "tsAIT.h"
#include "tsBinaryTable.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"AIT"
#define MY_TID ts::TID_AIT

TS_XML_TABLE_FACTORY(ts::AIT, MY_XML_NAME);
TS_ID_TABLE_FACTORY(ts::AIT, MY_TID);
TS_ID_SECTION_DISPLAY(ts::AIT::DisplaySection, MY_TID);

//----------------------------------------------------------------------------
// Constructors:
//----------------------------------------------------------------------------

ts::AIT::AIT(uint8_t version_, bool is_current_, uint16_t application_type_, bool test_application_)
    : AbstractLongTable(MY_TID, MY_XML_NAME, version_, is_current_)
    , application_type(application_type_)
    , test_application_flag(test_application_)
    , descs(this)
    , applications(this)
{
    _is_valid = true;
}

ts::AIT::AIT(const BinaryTable& table, const DVBCharset* charset)
    : AbstractLongTable(MY_TID, MY_XML_NAME)
    , application_type(0)
    , test_application_flag(false)
    , descs(this)
    , applications(this)
{
    deserialize(table, charset);
}

ts::AIT::AIT(const AIT& other)
    : AbstractLongTable(other)
    , application_type(other.application_type)
    , test_application_flag(other.test_application_flag)
    , descs(this, other.descs)
    , applications(this, other.applications)
{
}

//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AIT::deserialize(const BinaryTable& table, const DVBCharset* charset)
{
    // Clear table content
    _is_valid = false;
    application_type = 0;
    test_application_flag = false;
    descs.clear();
    applications.clear();

    if (!table.isValid() || table.tableId() != _table_id) {
        return;
    }

    // Loop on all sections.
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        uint16_t tid_ext(sect.tableIdExtension());
        test_application_flag = (tid_ext & 0x8000) != 0;
        application_type = tid_ext & 0x7fff;

        // Analyze the section payload:
        const uint8_t* data(sect.payload());
        size_t remain(sect.payloadSize());

        // Get ait common descriptor list
        if (remain < 2) {
            return;
        }
        size_t descriptors_length(GetUInt16(data) & 0x0FFF);
        data += 2;
        remain -= 2;
        descriptors_length = std::min(descriptors_length, remain);
        descs.add(data, descriptors_length);
        data += descriptors_length;
        remain -= descriptors_length;

        // Get application loop length
        if (remain < 2) {
            return;
        }
        size_t app_loop_length = (GetUInt16(data) & 0x0FFF);
        data += 2;
        remain -= 2;
        remain = std::min(app_loop_length, remain);

        // Get applications
        while (remain >= 9) {
            ApplicationIdentifier app_id(GetUInt32(data), GetUInt16(data + 4));
            Application& app(applications[app_id]);
            app.control_code = data[6];
            descriptors_length = GetUInt16(data + 7) & 0x0FFF;
            data += 9;
            remain -= 9;
            descriptors_length = std::min(descriptors_length, remain);
            app.descs.add(data, descriptors_length);
            data += descriptors_length;
            remain -= descriptors_length;
        }
    }

    _is_valid = true;
}

//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AIT::serialize(BinaryTable& table, const DVBCharset* charset) const
{
    // Reinitialize table object
    table.clear();

    // Return an empty table if not valid
    if (!_is_valid) {
        return;
    }

    // Current limitation: only one section is serialized.
    // Extraneous descriptors are dropped.

    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    uint8_t* data(payload);
    size_t remain(sizeof(payload));

    // Insert common descriptors list (with leading length field).
    // Provision space for 16-bit application loop length.
    remain -= 2;
    descs.lengthSerialize(data, remain);

    // Reserve placeholder for 16-bit application loop length.
    // Remain is unmodified here, already reserved before serializing the descriptors.
    uint8_t* const app_length = data;
    data += 2;

    // Add description of all applications
    for (ApplicationMap::const_iterator it = applications.begin(); it != applications.end() && remain >= 9; ++it) {

        // Insert stream type and pid
        PutUInt32(data, it->first.organization_id);
        PutUInt16(data + 4, it->first.application_id);
        data[6] = it->second.control_code;
        data += 7;
        remain -= 7;

        // Insert application descriptors list (with leading length field)
        it->second.descs.lengthSerialize(data, remain);
    }

    // Now update the 16-bit application loop length.
    PutUInt16(app_length, uint16_t(0xF000 | (data - app_length - 2)));

    // Compute synthetic tid extension.
    uint16_t tid_ext = (test_application_flag ? 0x8000 : 0x0000) | (application_type & 0x7FFF);

    // Add one single section in the table
    table.addSection(new Section(MY_TID, // tid
        true,                            // is_private_section
        tid_ext,                         // tid_ext
        version,
        is_current,
        0, // section_number,
        0, // last_section_number
        payload,
        data - payload)); // payload_size,
}

//----------------------------------------------------------------------------
// A static method to display a AIT section.
//----------------------------------------------------------------------------

void ts::AIT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');
    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    uint16_t application_type = section.tableIdExtension() & 0x7FFF;
    bool test_application_flag = (section.tableIdExtension() & 0x8000) != 0;
    strm << margin << UString::Format(u"Application type: %d (0x%X)", { application_type, application_type });
    strm << u", Test application: " << test_application_flag << std::endl;

    if (size >= 4) {
        size_t length_field = GetUInt16(data) & 0x0FFF; // common_descriptors_length
        data += 2;
        size -= 2;
        length_field = std::min(size, length_field);

        // Process and display "common descriptors loop"
        if (length_field > 0) {
            strm << margin << "Common descriptor loop:" << std::endl;
            display.displayDescriptorList(data, length_field, indent, section.tableId());
        }
        data += length_field;
        size -= length_field;

        if (size >= 2) {
            length_field = GetUInt16(data) & 0x0FFF; // application_loop_length
            data += 2;
            size -= 2;
            length_field = std::min(size, length_field);

            // Process and display "application loop"
            while (size >= 9) {
                uint32_t org_id = GetUInt32(data);
                uint16_t app_id = GetUInt16(data + 4);
                uint8_t control_code = *(data + 6);
                length_field = GetUInt16(data + 7) & 0xFFF; // application_descriptors_loop_length
                data += 9;
                size -= 9;
                length_field = std::min(size, length_field);

                strm << margin << "Application: Identifier: (Organization id: " << UString::Format(u"%d (0x%X)", { org_id, org_id })
                     << ", Application id: " << UString::Format(u"%d (0x%X)", { app_id, app_id })
                     << UString::Format(u"), Control code: %d", { control_code })
                     << std::endl;
                display.displayDescriptorList(data, length_field, indent, section.tableId());
                data += length_field;
                size -= length_field;
            }
        }
    }

    display.displayExtraData(data, size, indent);
}

//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AIT::buildXML(xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setBoolAttribute(u"test_application_flag", test_application_flag);
    root->setIntAttribute(u"application_type", application_type, true);
    descs.toXML(root);

    for (ApplicationMap::const_iterator it = applications.begin(); it != applications.end(); ++it) {
        xml::Element* e = root->addElement(u"application");
        e->setIntAttribute(u"control_code", it->second.control_code, true);
        xml::Element* id = e->addElement(u"application_identifier");
        id->setIntAttribute(u"organization_id", it->first.organization_id, true);
        id->setIntAttribute(u"application_id", it->first.application_id, true);

        it->second.descs.toXML(e);
    }
}

//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::AIT::fromXML(const xml::Element* element)
{
    descs.clear();
    applications.clear();

    xml::ElementVector children;
    _is_valid = checkXMLName(element) && element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) && element->getBoolAttribute(is_current, u"current", false, true) && element->getBoolAttribute(test_application_flag, u"test_application_flag", false, true) && element->getIntAttribute<uint16_t>(application_type, u"application_type", true, 0, 0x0000, 0x7FFF) && descs.fromXML(children, element, u"application");

    // Iterate through applications
    for (size_t index = 0; _is_valid && index < children.size(); ++index) {
        Application application(this);
        ApplicationIdentifier identifier;
        const xml::Element* id = children[index]->findFirstChild(u"application_identifier", true);

        xml::ElementVector others;
        UStringList allowed({ u"application_identifier" });

        _is_valid = children[index]->getIntAttribute<uint8_t>(application.control_code, u"control_code", true, 0, 0x00, 0xFF) &&
            application.descs.fromXML(others, children[index], allowed) &&
            id != nullptr &&
            id->getIntAttribute<uint32_t>(identifier.organization_id, u"organization_id", true, 0, 0, 0xFFFFFFFF) &&
            id->getIntAttribute<uint16_t>(identifier.application_id, u"application_id", true, 0, 0, 0xFFFF);

        if (_is_valid) {
            applications[identifier] = application;
        }
    }
}
