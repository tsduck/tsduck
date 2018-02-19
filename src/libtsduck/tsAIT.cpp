//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Tristan Claverie
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
// Default constructor:
//----------------------------------------------------------------------------

ts::AIT::AIT(uint8_t version_, bool is_current_, uint16_t application_type_, bool test_application_)
    : AbstractLongTable(MY_TID, MY_XML_NAME, version_, is_current_)
    , application_type(application_type_)
    , test_application_flag(test_application_)
    , descs()
    , applications()
{
    _is_valid = true;
}

//----------------------------------------------------------------------------
// Constructor from a binary table
//----------------------------------------------------------------------------

ts::AIT::AIT(const BinaryTable& table, const DVBCharset* charset)
    : AbstractLongTable(MY_TID, MY_XML_NAME)
    , application_type(0)
    , test_application_flag(false)
    , descs()
    , applications()
{
    deserialize(table, charset);
}

//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AIT::deserialize(const BinaryTable& table, const DVBCharset* charset)
{
    uint16_t tmpUint16;
    // Clear table content
    _is_valid = false;
    application_type = 0;
    test_application_flag = false;
    descs.clear();
    applications.clear();

    if (!table.isValid() || table.tableId() != _table_id) {
        return;
    }

    // TODO Deal with sections
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        tmpUint16 = sect.tableIdExtension();
        test_application_flag = (tmpUint16 & 0x8000) > 0;
        application_type = tmpUint16 & 0x7fff;

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
            app.control_code = data[5];
            descriptors_length = GetUInt16(data + 6) & 0x0FFF;
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

    // TODO section
    uint8_t payload[MAX_PSI_LONG_SECTION_PAYLOAD_SIZE];
    uint8_t* data(payload);
    size_t remain(sizeof(payload));

    // Insert common descriptors list (with leading length field)
    descs.lengthSerialize(data, remain);

    // Add description of all applications
    for (ApplicationMap::const_iterator it = applications.begin(); it != applications.end() && remain >= 9; ++it) {

        // Insert stream type and pid
        PutUInt32(data, it->first.organisation_id);
        PutUInt16(data + 4, it->first.application_id);
        data[6] = it->second.control_code;
        data += 7;
        remain -= 7;

        // Insert application descriptors list (with leading length field)
        size_t next_index = it->second.descs.lengthSerialize(data, remain);
        // TODO Handle sections
        if (next_index != it->second.descs.count()) {
            // Not enough space to serialize all descriptors in the section.
            // A PMT cannot have more than one section.
            // Return with table left in invalid state.
            return;
        }
    }

    uint16_t tid_ext = test_application_flag << 15 | application_type;

    // Add one single section in the table
    table.addSection(new Section(MY_TID, // tid
        false,                           // is_private_section
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

    // TODO Check size for AIT
    if (size >= 4) {
        // Fixed part
        PID pid = GetUInt16(data) & 0x1FFF;
        size_t length_field = GetUInt16(data + 2) & 0x0FFF; // section_length
        data += 4;
        size -= 4;
        if (length_field > size) {
            length_field = size;
        }
        uint16_t application_type = section.tableIdExtension() & 0x7FFF;
        bool test_application_flag = (section.tableIdExtension() & 0x8000) > 0;
        strm << margin << UString::Format(u"Application type: %d (0x%X)", { application_type, application_type });
        strm << u", Test application: " << test_application_flag << std::endl;

        // Process and display "common descriptors loop"
        if (length_field > 0) {
            strm << margin << "Common descriptor loop:" << std::endl;
            display.displayDescriptorList(data, length_field, indent, section.tableId());
        }
        data += length_field;
        size -= length_field;

        // Process and display "application loop"
        strm << margin << "Applications:" << std::endl;
        while (size >= 9) {
            // TODO fill in this loop for displaying applications
            uint8_t stream = *data;
            PID es_pid = GetUInt16(data + 1) & 0x1FFF;
            size_t es_info_length = GetUInt16(data + 3) & 0x0FFF;
            data += 5;
            size -= 5;
            if (es_info_length > size) {
                es_info_length = size;
            }
            strm << margin << "Elementary stream: type " << names::StreamType(stream, names::FIRST)
                 << ", PID: " << es_pid << UString::Format(u" (0x%X)", { es_pid }) << std::endl;
            display.displayDescriptorList(data, es_info_length, indent, section.tableId());
            data += es_info_length;
            size -= es_info_length;
        }
    }

    display.displayExtraData(data, size, indent);
}

//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::PMT::buildXML(xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"service_id", service_id, true);
    if (pcr_pid != PID_NULL) {
        root->setIntAttribute(u"PCR_PID", pcr_pid, true);
    }
    descs.toXML(root);

    for (StreamMap::const_iterator it = streams.begin(); it != streams.end(); ++it) {
        xml::Element* e = root->addElement(u"component");
        e->setIntAttribute(u"elementary_PID", it->first, true);
        e->setIntAttribute(u"stream_type", it->second.stream_type, true);
        it->second.descs.toXML(e);
    }
}

//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::PMT::fromXML(const xml::Element* element)
{
    descs.clear();
    streams.clear();

    xml::ElementVector children;
    _is_valid = checkXMLName(element) && element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) && element->getBoolAttribute(is_current, u"current", false, true) && element->getIntAttribute<uint16_t>(service_id, u"service_id", true, 0, 0x0000, 0xFFFF) && element->getIntAttribute<PID>(pcr_pid, u"PCR_PID", false, PID_NULL, 0x0000, 0x1FFF) && descs.fromXML(children, element, u"component");

    for (size_t index = 0; _is_valid && index < children.size(); ++index) {
        PID pid = PID_NULL;
        Stream stream;
        _is_valid = children[index]->getIntAttribute<uint8_t>(stream.stream_type, u"stream_type", true, 0, 0x00, 0xFF) && children[index]->getIntAttribute<PID>(pid, u"elementary_PID", true, 0, 0x0000, 0x1FFF) && stream.descs.fromXML(children[index]);
        if (_is_valid) {
            streams[pid] = stream;
        }
    }
}
