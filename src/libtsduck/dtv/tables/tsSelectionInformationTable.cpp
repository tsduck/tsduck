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

#include "tsSelectionInformationTable.h"
#include "tsRST.h"
#include "tsNames.h"
#include "tsBinaryTable.h"
#include "tsStreamIdentifierDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"selection_information_table"
#define MY_CLASS ts::SelectionInformationTable
#define MY_TID ts::TID_SIT
#define MY_PID ts::PID_SIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SelectionInformationTable::SelectionInformationTable(uint8_t version_, bool is_current_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, is_current_),
    descs(this),
    services(this)
{
}

ts::SelectionInformationTable::SelectionInformationTable(const SelectionInformationTable& other) :
    AbstractLongTable(other),
    descs(this, other.descs),
    services(this, other.services)
{
}

ts::SelectionInformationTable::SelectionInformationTable(DuckContext& duck, const BinaryTable& table) :
    SelectionInformationTable()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::SelectionInformationTable::tableIdExtension() const
{
    return 0xFFFF;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::SelectionInformationTable::clearContent()
{
    descs.clear();
    services.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SelectionInformationTable::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Clear table content
    descs.clear();
    services.clear();

    // Loop on all sections, although a Selection Information Table is not allowed
    // to use more than one section, see ETSI EN 300 468, 7.1.2.
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();

        // Get global  descriptor list
        if (remain < 2) {
            return;
        }
        size_t info_length = GetUInt16(data) & 0x0FFF;
        data += 2;
        remain -= 2;
        info_length = std::min(info_length, remain);
        descs.add(data, info_length);
        data += info_length; remain -= info_length;

        // Get service description
        while (remain >= 4) {
            const uint16_t id = GetUInt16(data);
            Service& srv(services[id]);
            srv.running_status = (data[2] >> 4) & 0x07;
            info_length = GetUInt16(data + 2) & 0x0FFF;
            data += 4; remain -= 4;
            info_length = std::min(info_length, remain);
            srv.descs.add(data, info_length);
            data += info_length; remain -= info_length;
        }
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SelectionInformationTable::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the section. Note that a Selection Information Table is not allowed
    // to use more than one section, see ETSI EN 300 468, 7.1.2.
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    uint8_t* data = payload;
    size_t remain = sizeof(payload);

    // Insert program_info descriptor list (with leading length field)
    descs.lengthSerialize(data, remain);

    // Add description of all services.
    for (auto it = services.begin(); it != services.end() && remain >= 4; ++it) {

        // Insert stream type and pid
        PutUInt16(data, it->first); // service id
        data += 2; remain -= 2;

        // Insert descriptor list for service (with leading length field)
        size_t next_index = it->second.descs.lengthSerialize(data, remain, 0, it->second.running_status | 0x08);
        if (next_index != it->second.descs.count()) {
            // Not enough space to serialize all descriptors in the section.
            // A SelectionInformationTable cannot have more than one section.
            // Return with table left in invalid state.
            return;
        }
    }

    // Add one single section in the table
    table.addSection(new Section(MY_TID,           // tid
                                 true,             // is_private_section
                                 0xFFFF,           // tid_ext
                                 version,
                                 is_current,
                                 0,                // section_number,
                                 0,                // last_section_number
                                 payload,
                                 data - payload)); // payload_size,
}


//----------------------------------------------------------------------------
// A static method to display a SelectionInformationTable section.
//----------------------------------------------------------------------------

void ts::SelectionInformationTable::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    if (size >= 2) {
        // Fixed part
        size_t info_length = GetUInt16(data) & 0x0FFF;
        data += 2; size -= 2;
        if (info_length > size) {
            info_length = size;
        }

        // Process and display global descriptor list.
        if (info_length > 0) {
            strm << margin << "Global information:" << std::endl;
            display.displayDescriptorList(section, data, info_length, indent);
        }
        data += info_length; size -= info_length;

        // Process and display "service info"
        while (size >= 4) {
            const uint16_t id = GetUInt16(data);
            const uint8_t rs = (data[2] >> 4) & 0x07;
            info_length = GetUInt16(data + 2) & 0x0FFF;
            data += 4; size -= 4;
            if (info_length > size) {
                info_length = size;
            }
            strm << margin << UString::Format(u"Service id: %d (0x%X), Status: %s", {id, id, RST::RunningStatusNames.name(rs)}) << std::endl;
            display.displayDescriptorList(section, data, info_length, indent);
            data += info_length; size -= info_length;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SelectionInformationTable::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    descs.toXML(duck, root);

    for (auto it = services.begin(); it != services.end(); ++it) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"service_id", it->first, true);
        e->setEnumAttribute(RST::RunningStatusNames, u"running_status", it->second.running_status);
        it->second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SelectionInformationTable::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        descs.fromXML(duck, children, element, u"service");

    for (size_t index = 0; ok && index < children.size(); ++index) {
        uint16_t id = 0;
        ok = children[index]->getIntAttribute<uint16_t>(id, u"service_id", true) &&
             children[index]->getIntEnumAttribute<uint8_t>(services[id].running_status, RST::RunningStatusNames, u"running_status", true);
             services[id].descs.fromXML(duck, children[index]);
    }
    return ok;
}
