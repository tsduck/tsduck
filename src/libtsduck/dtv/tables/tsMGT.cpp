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

#include "tsMGT.h"
#include "tsNames.h"
#include "tsBinaryTable.h"
#include "tsStreamIdentifierDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"MGT"
#define MY_TID ts::TID_MGT
#define MY_STD ts::Standards::ATSC

TS_REGISTER_TABLE(ts::MGT, {MY_TID}, MY_STD, MY_XML_NAME, ts::MGT::DisplaySection, nullptr, {ts::PID_PSIP});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MGT::MGT(uint8_t version_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, true), // MGT is always "current"
    protocol_version(0),
    tables(this),
    descs(this)
{
}

ts::MGT::MGT(const MGT& other) :
    AbstractLongTable(other),
    protocol_version(other.protocol_version),
    tables(this, other.tables),
    descs(this, other.descs)
{
}

ts::MGT::MGT(DuckContext& duck, const BinaryTable& table) :
    MGT()
{
    deserialize(duck, table);
}

ts::MGT::TableType::TableType(const AbstractTable* table) :
    EntryWithDescriptors(table),
    table_type(0),
    table_type_PID(PID_NULL),
    table_type_version_number(0),
    number_bytes(0)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::MGT::tableIdExtension() const
{
    return 0;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::MGT::clearContent()
{
    protocol_version = 0;
    tables.clear();
    descs.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MGT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Clear table content
    protocol_version = 0;
    descs.clear();
    tables.clear();

    // Loop on all sections (although a MGT is not allowed to use more than one section, see A/65, section 6.2)
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent(); // should be true

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();
        if (remain < 3) {
            return; // invalid table, too short
        }

        // Get fixed fields.
        protocol_version = data[0];
        uint16_t tables_defined = GetUInt16(data + 1);
        data += 3; remain -= 3;

        // Loop on all table types definitions.
        while (tables_defined > 0 && remain >= 11) {
            // Add a new TableType at the end of the list.
            TableType& tt(tables.newEntry());
            tt.table_type = GetUInt16(data);
            tt.table_type_PID = GetUInt16(data + 2) & 0x1FFF;
            tt.table_type_version_number = data[4] & 0x1F;
            tt.number_bytes = GetUInt32(data + 5);
            size_t info_length = GetUInt16(data + 9) & 0x0FFF;
            data += 11; remain -= 11;
            info_length = std::min(info_length, remain);
            tt.descs.add(data, info_length);
            data += info_length; remain -= info_length;
            tables_defined--;
        }
        if (tables_defined > 0 || remain < 2) {
            return; // truncated table.
        }

        // Get program information descriptor list
        size_t info_length = GetUInt16(data) & 0x0FFF;
        data += 2; remain -= 2;
        info_length = std::min(info_length, remain);
        descs.add(data, info_length);
        data += info_length;
        remain -= info_length;
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MGT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the section. Note that a MGT is not allowed to use more than one section, see A/65, section 6.2.
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    uint8_t* data = payload;
    size_t remain = sizeof(payload);

    // Add fixed fields.
    data[0] = protocol_version;
    PutUInt16(data + 1, uint16_t(tables.size()));
    data += 3; remain -= 3;

    // Add description of all table types.
    for (auto it = tables.begin(); it != tables.end() && remain >= 11; ++it) {
        const TableType& tt(it->second);

        // Insert fixed fields.
        PutUInt16(data, tt.table_type);
        PutUInt16(data + 2, 0xE000 | tt.table_type_PID);
        PutUInt8(data + 4, 0xE0 | tt.table_type_version_number);
        PutUInt32(data + 5, tt.number_bytes);
        data += 9; remain -= 9;

        // Insert descriptor list for this table type (with leading length field)
        size_t next_index = tt.descs.lengthSerialize(data, remain);
        if (next_index != tt.descs.count()) {
            // Not enough space to serialize all descriptors in the section.
            // A MGT cannot have more than one section.
            // Return with table left in invalid state.
            return;
        }
    }

    // Insert common descriptor list (with leading length field)
    descs.lengthSerialize(data, remain);

    // Add one single section in the table
    table.addSection(new Section(MY_TID,           // tid
                                 true,             // is_private_section
                                 0,                // tid_ext is always zero in MGT
                                 version,
                                 is_current,       // should be true
                                 0,                // section_number,
                                 0,                // last_section_number
                                 payload,
                                 data - payload)); // payload_size,
}


//----------------------------------------------------------------------------
// An Enumeration object for table_type.
// Need a specific constructor because of the large list of values.
//----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::MGT::TableTypeEnum);

ts::MGT::TableTypeEnum::TableTypeEnum() :
    Enumeration({
        {u"TVCT-current", 0x0000},
        {u"TVCT-next",    0x0001},
        {u"CVCT-current", 0x0002},
        {u"CVCT-next",    0x0003},
        {u"ETT",          0x0004},
        {u"DCCSCT",       0x0005},
    })
{
    // 0x0100-0x017F EIT-0 to EIT-127
    for (int val = 0x0100; val <= 0x017F; ++val) {
        add(UString::Format(u"EIT-%d", {val & 0x00FF}), val);
    }
    // 0x0200 - 0x027F Event ETT - 0 to event ETT - 127
    for (int val = 0x0200; val <= 0x027F; ++val) {
        add(UString::Format(u"ETT-%d", {val & 0x00FF}), val);
    }
    // 0x0301 - 0x03FF RRT with rating_region 1 - 255
    for (int val = 0x0301; val <= 0x03FF; ++val) {
        add(UString::Format(u"RRT-%d", {val & 0x00FF}), val);
    }
    // 0x1400 - 0x14FF DCCT with dcc_id 0x00 - 0xFF
    for (int val = 0x1400; val <= 0x14FF; ++val) {
        add(UString::Format(u"DCCT-%d", {val & 0x00FF}), val);
    }
}


//----------------------------------------------------------------------------
// Get the name for a 16-bit table type from an MGT.
//----------------------------------------------------------------------------

ts::UString ts::MGT::TableTypeName(uint16_t table_type)
{
    return TableTypeEnum::Instance()->name(table_type);
}


//----------------------------------------------------------------------------
// A static method to display a MGT section.
//----------------------------------------------------------------------------

void ts::MGT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    if (size >= 3) {
        // Fixed part.
        uint16_t table_count = GetUInt16(data + 1);
        strm << margin << UString::Format(u"Protocol version: %d, number of table types: %d", {data[0], table_count}) << std::endl;
        data += 3; size -= 3;

        // Display all table types.
        while (table_count > 0 && size >= 11) {

            const uint16_t type = GetUInt16(data);
            const PID pid = GetUInt16(data + 2) & 0x1FFF;
            strm << margin << UString::Format(u"- Table type: %s (0x%X)", {TableTypeName(type), type}) << std::endl
                 << margin << UString::Format(u"  PID: 0x%X (%d), version: %d, size: %d bytes", {pid, pid, data[4] & 0x1F, GetUInt32(data + 5)}) << std::endl;

            // Use fake PDS for ATSC.
            size_t info_length = GetUInt16(data + 9) & 0x0FFF;
            data += 11; size -= 11;
            info_length = std::min(info_length, size);
            display.displayDescriptorList(section, data, info_length, indent + 2);

            data += info_length; size -= info_length;
            table_count--;
        }

        // Display common descriptors. Use fake PDS for ATSC.
        if (table_count == 0 && size >= 2) {
            size_t info_length = GetUInt16(data) & 0x0FFF;
            data += 2; size -= 2;
            info_length = std::min(info_length, size);
            if (info_length > 0) {
                strm << margin << "- Global descriptors:" << std::endl;
                display.displayDescriptorList(section, data, info_length, indent + 2);
                data += info_length; size -= info_length;
            }
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MGT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setIntAttribute(u"protocol_version", protocol_version);
    descs.toXML(duck, root);

    for (auto it = tables.begin(); it != tables.end(); ++it) {
        xml::Element* e = root->addElement(u"table");
        e->setEnumAttribute(*TableTypeEnum::Instance(), u"type", it->second.table_type);
        e->setIntAttribute(u"PID", it->second.table_type_PID, true);
        e->setIntAttribute(u"version_number", it->second.table_type_version_number);
        e->setIntAttribute(u"number_bytes", it->second.number_bytes);
        it->second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MGT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getIntAttribute<uint8_t>(protocol_version, u"protocol_version", false, 0) &&
        descs.fromXML(duck, children, element, u"table");

    for (size_t index = 0; ok && index < children.size(); ++index) {
        // Add a new TableType at the end of the list.
        TableType& tt(tables.newEntry());
        ok = children[index]->getIntEnumAttribute(tt.table_type, *TableTypeEnum::Instance(), u"type", true) &&
             children[index]->getIntAttribute<PID>(tt.table_type_PID, u"PID", true, 0, 0x0000, 0x1FFF) &&
             children[index]->getIntAttribute<uint8_t>(tt.table_type_version_number, u"version_number", true, 0, 0, 31) &&
             children[index]->getIntAttribute<uint32_t>(tt.number_bytes, u"number_bytes", true) &&
             tt.descs.fromXML(duck, children[index]);
    }
    return ok;
}
