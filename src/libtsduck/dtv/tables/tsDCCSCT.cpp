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

#include "tsDCCSCT.h"
#include "tsNames.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"DCCSCT"
#define MY_CLASS ts::DCCSCT
#define MY_TID ts::TID_DCCSCT
#define MY_STD ts::Standards::ATSC

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);

const ts::Enumeration ts::DCCSCT::UpdateTypeNames({
    {u"new_genre_category", ts::DCCSCT::new_genre_category},
    {u"new_state",          ts::DCCSCT::new_state},
    {u"new_county",         ts::DCCSCT::new_county},
});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DCCSCT::DCCSCT(uint8_t version_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, true), // DCCSCT is always "current"
    dccsct_type(0),
    protocol_version(0),
    updates(this),
    descs(this)
{
}

ts::DCCSCT::DCCSCT(const DCCSCT& other) :
    AbstractLongTable(other),
    dccsct_type(other.dccsct_type),
    protocol_version(other.protocol_version),
    updates(this, other.updates),
    descs(this, other.descs)
{
}

ts::DCCSCT::DCCSCT(DuckContext& duck, const BinaryTable& table) :
    DCCSCT()
{
    deserialize(duck, table);
}

ts::DCCSCT::Update::Update(const AbstractTable* table, UpdateType type) :
    EntryWithDescriptors(table),
    update_type(type),
    genre_category_code(0),
    genre_category_name_text(),
    dcc_state_location_code(0),
    dcc_state_location_code_text(),
    state_code(0),
    dcc_county_location_code(0),
    dcc_county_location_code_text()
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::DCCSCT::tableIdExtension() const
{
    return dccsct_type;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::DCCSCT::clearContent()
{
    dccsct_type = 0;
    protocol_version = 0;
    descs.clear();
    updates.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DCCSCT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    clear();

    // Loop on all sections (although a DCCSCT is not allowed to use more than one section, see A/65, section 6.8)
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent(); // should be true
        dccsct_type = sect.tableIdExtension();

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();
        if (remain < 2) {
            return; // invalid table, too short
        }

        // Get fixed fields.
        protocol_version = data[0];
        uint8_t updates_defined = data[1];
        data += 2; remain -= 2;

        // Loop on all table types definitions.
        while (updates_defined > 0 && remain >= 2) {
            // Add a new Update at the end of the list.
            Update& upd(updates.newEntry());

            upd.update_type = UpdateType(data[0]);
            size_t len = data[1];
            data += 2; remain -= 2;

            if (remain < len) {
                return; // invalid table, too short
            }
            switch (upd.update_type) {
                case new_genre_category: {
                    if (len < 2) {
                        return; // invalid table, too short
                    }
                    upd.genre_category_code = data[0];
                    data++; remain--;
                    if (!upd.genre_category_name_text.deserialize(duck, data, remain, len - 1)) {
                        return; // invalid table
                    }
                    break;
                }
                case new_state: {
                    if (len < 2) {
                        return; // invalid table, too short
                    }
                    upd.dcc_state_location_code = data[0];
                    data++; remain--;
                    if (!upd.dcc_state_location_code_text.deserialize(duck, data, remain, len - 1)) {
                        return; // invalid table
                    }
                    break;
                }
                case new_county: {
                    if (len < 4) {
                        return; // invalid table, too short
                    }
                    upd.state_code = data[0];
                    upd.dcc_county_location_code = GetUInt16(data + 1) & 0x03FF;
                    data += 3; remain -= 3;
                    if (!upd.dcc_county_location_code_text.deserialize(duck, data, remain, len - 3)) {
                        return; // invalid table
                    }
                    break;
                }
                default: {
                    data += len; remain -= len;
                    break;
                }
            }

            // Deserialize descriptor list for this update.
            if (remain < 2) {
                return; // invalid table, too short
            }
            len = GetUInt16(data) & 0x03FF;
            data += 2; remain -= 2;
            len = std::min(len, remain);
            upd.descs.add(data, len);
            data += len; remain -= len;

            updates_defined--;
        }
        if (updates_defined > 0 || remain < 2) {
            return; // truncated table.
        }

        // Get descriptor list for the global table.
        size_t len = GetUInt16(data) & 0x03FF;
        data += 2; remain -= 2;
        len = std::min(len, remain);
        descs.add(data, len);
        data += len; remain -= len;
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DCCSCT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the section. Note that a DCCSCT is not allowed to use more than one section, see A/65, section 6.2.
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    uint8_t* data = payload;
    size_t remain = sizeof(payload);

    // Add fixed fields.
    data[0] = protocol_version;
    data[1] = uint8_t(updates.size());
    data += 2; remain -= 2;

    // Add description of all updates.
    for (auto it = updates.begin(); it != updates.end() && remain >= 2; ++it) {
        const Update& upd(it->second);

        // Insert fixed fields.
        data[0] = uint8_t(upd.update_type);
        data[1] = 0; // place-holder for update_data_length
        uint8_t* data_length_addr = data + 1;
        data += 2; remain -= 2;

        // We always need at least 4 bytes for the length of the two descriptor lists.
        if (remain < 4) {
            return;
        }

        // Insert type-dependent data. We now know that we have enough space for the fixed parts.
        switch (upd.update_type) {
            case new_genre_category: {
                data[0] = upd.genre_category_code;
                data++; remain--;
                upd.genre_category_name_text.serialize(duck, data, remain);
                break;
            }
            case new_state: {
                data[0] = upd.dcc_state_location_code;
                data++; remain--;
                upd.dcc_state_location_code_text.serialize(duck, data, remain);
                break;
            }
            case new_county: {
                data[0] = upd.state_code;
                PutUInt16(data + 1, 0xFC00 | upd.dcc_county_location_code);
                data += 3; remain -= 3;
                upd.dcc_county_location_code_text.serialize(duck, data, remain);
                break;
            }
            default: {
                break;
            }
        }

        // Update update_data_length
        *data_length_addr = uint8_t(data - data_length_addr - 1);

        // Insert descriptor list for this updates (with leading length field)
        size_t next_index = upd.descs.lengthSerialize(data, remain, 0, 0x003F, 10);
        if (next_index < upd.descs.count()) {
            // Not enough space to serialize all descriptors in the section.
            return;
        }
    }

    // Insert common descriptor list (with leading length field)
    size_t next_index = descs.lengthSerialize(data, remain, 0, 0x003F, 10);
    if (next_index < descs.count()) {
        // Not enough space to serialize all descriptors in the section.
        return;
    }

    // Add one single section in the table
    table.addSection(new Section(MY_TID,           // tid
                                 true,             // is_private_section
                                 dccsct_type,      // tid_ext
                                 version,
                                 is_current,       // should be true
                                 0,                // section_number,
                                 0,                // last_section_number
                                 payload,
                                 data - payload)); // payload_size,
}


//----------------------------------------------------------------------------
// A static method to display a DCCSCT section.
//----------------------------------------------------------------------------

void ts::DCCSCT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    if (size >= 2) {
        // Fixed part.
        size_t updates_defined = data[1];
        strm << margin << UString::Format(u"Protocol version: %d, DCCSCT type: 0x%X, number of updates: %d", {data[0], section.tableIdExtension(), updates_defined}) << std::endl;
        data += 2; size -= 2;

        // Loop on all update.
        while (updates_defined > 0 && size >= 2) {

            const uint8_t utype = data[0];
            size_t len = data[1];
            data += 2; size -= 2;

            strm << margin << UString::Format(u"- Update type: 0x%X (%s)", {utype, UpdateTypeNames.name(utype)}) << std::endl;

            if (size < len) {
                break;
            }

            // Display variable part.
            switch (utype) {
                case new_genre_category: {
                    if (len >= 1) {
                        strm << margin << UString::Format(u"  Genre category code: 0x%X (%d)", {data[0], data[0]}) << std::endl;
                        data++; size--;
                        ATSCMultipleString::Display(display, u"Genre category name: ", indent + 2, data, size, len - 1);
                    }
                    break;
                }
                case new_state: {
                    if (len >= 1) {
                        strm << margin << UString::Format(u"  DCC state location code: 0x%X (%d)", {data[0], data[0]}) << std::endl;
                        data++; size--;
                        ATSCMultipleString::Display(display, u"DCC state location: ", indent + 2, data, size, len - 1);
                    }
                    break;
                }
                case new_county: {
                    if (len >= 3) {
                        const uint16_t county = GetUInt16(data + 1) & 0x03FF;
                        strm << margin << UString::Format(u"  State code: 0x%X (%d), DCC county location code: 0x%03X (%d)", {data[0], data[0], county, county}) << std::endl;
                        data += 3; size -= 3;
                        ATSCMultipleString::Display(display, u"DCC county location: ", indent + 2, data, size, len - 3);
                    }
                    break;
                }
                default: {
                    display.displayExtraData(data, len, indent + 2);
                    data += len; size -= len;
                    break;
                }
            }

            // Display descriptor list for this update.
            if (size >= 2) {
                len = GetUInt16(data) & 0x03FF;
                data += 2; size -= 2;
                len = std::min(len, size);
                if (len > 0) {
                    strm << margin << "  Descriptors for this update:" << std::endl;
                    display.displayDescriptorList(section, data, len, indent + 2);
                    data += len; size -= len;
                }
            }

            updates_defined--;
        }

        // Display descriptor list for the global table.
        if (size >= 2) {
            size_t len = GetUInt16(data) & 0x03FF;
            data += 2; size -= 2;
            len = std::min(len, size);
            if (len > 0) {
                strm << margin << "Additional descriptors:" << std::endl;
                display.displayDescriptorList(section, data, len, indent);
                data += len; size -= len;
            }
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DCCSCT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setIntAttribute(u"protocol_version", protocol_version);
    root->setIntAttribute(u"dccsct_type", dccsct_type, true);
    descs.toXML(duck, root);

    for (auto upd = updates.begin(); upd != updates.end(); ++upd) {
        xml::Element* e = root->addElement(u"update");
        e->setEnumAttribute(UpdateTypeNames, u"update_type", upd->second.update_type);
        upd->second.descs.toXML(duck, e);
        switch (upd->second.update_type) {
            case new_genre_category: {
                e->setIntAttribute(u"genre_category_code", upd->second.genre_category_code, true);
                upd->second.genre_category_name_text.toXML(duck, e, u"genre_category_name_text", false);
                break;
            }
            case new_state: {
                e->setIntAttribute(u"dcc_state_location_code", upd->second.dcc_state_location_code, true);
                upd->second.dcc_state_location_code_text.toXML(duck, e, u"dcc_state_location_code_text", false);
                break;
            }
            case new_county: {
                e->setIntAttribute(u"state_code", upd->second.state_code, true);
                e->setIntAttribute(u"dcc_county_location_code", upd->second.dcc_county_location_code, true);
                upd->second.dcc_county_location_code_text.toXML(duck, e, u"dcc_county_location_code_text", false);
                break;
            }
            default: {
                break;
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DCCSCT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getIntAttribute<uint8_t>(protocol_version, u"protocol_version", false, 0) &&
        element->getIntAttribute<uint16_t>(dccsct_type, u"dccsct_type", false, 0) &&
        descs.fromXML(duck, children, element, u"update");

    for (size_t index = 0; ok && index < children.size(); ++index) {
        // Add a new Update at the end of the list.
        Update& upd(updates.newEntry());
        xml::ElementVector unused;
        ok = children[index]->getIntEnumAttribute(upd.update_type, UpdateTypeNames, u"update_type", true) &&
            children[index]->getIntAttribute<uint8_t>(upd.genre_category_code, u"genre_category_code", upd.update_type == new_genre_category) &&
            children[index]->getIntAttribute<uint8_t>(upd.dcc_state_location_code, u"dcc_state_location_code", upd.update_type == new_state) &&
            children[index]->getIntAttribute<uint8_t>(upd.state_code, u"state_code", upd.update_type == new_county) &&
            children[index]->getIntAttribute<uint16_t>(upd.dcc_county_location_code, u"dcc_county_location_code", upd.update_type == new_county, 0, 0, 0x03FF) &&
            upd.genre_category_name_text.fromXML(duck, children[index], u"genre_category_name_text", upd.update_type == new_genre_category) &&
            upd.dcc_state_location_code_text.fromXML(duck, children[index], u"dcc_state_location_code_text", upd.update_type == new_state) &&
            upd.dcc_county_location_code_text.fromXML(duck, children[index], u"dcc_county_location_code_text", upd.update_type == new_county) &&
            upd.descs.fromXML(duck, unused, children[index], u"genre_category_name_text,dcc_state_location_code_text,dcc_county_location_code_text");
    }
    return ok;
}
