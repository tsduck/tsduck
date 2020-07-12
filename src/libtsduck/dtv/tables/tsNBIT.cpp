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

#include "tsNBIT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"NBIT"
#define MY_CLASS ts::NBIT
#define MY_PID ts::PID_NBIT
#define MY_STD ts::Standards::ISDB

TS_REGISTER_TABLE(MY_CLASS, {ts::TID_NBIT_BODY, ts::TID_NBIT_REF}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::NBIT::NBIT(bool is_body, uint8_t vers, bool cur) :
    AbstractLongTable(TID(is_body ? TID_NBIT_BODY : TID_NBIT_REF), MY_XML_NAME, MY_STD, vers, cur),
    original_network_id(0),
    informations(this)
{
}

ts::NBIT::NBIT(const NBIT& other) :
    AbstractLongTable(other),
    original_network_id(other.original_network_id),
    informations(this, other.informations)
{
}

ts::NBIT::NBIT(DuckContext& duck, const BinaryTable& table) :
    NBIT()
{
    deserialize(duck, table);
}

ts::NBIT::Information::Information(const AbstractTable* table) :
    EntryWithDescriptors(table),
    information_type(0),
    description_body_location(0),
    user_defined(0),
    key_ids()
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::NBIT::tableIdExtension() const
{
    return original_network_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::NBIT::clearContent()
{
    original_network_id = 0;
    informations.clear();
}


//----------------------------------------------------------------------------
// This method checks if a table id is valid for this object.
//----------------------------------------------------------------------------

bool ts::NBIT::isValidTableId(TID tid) const
{
    return tid == TID_NBIT_BODY || tid == TID_NBIT_REF;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::NBIT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    // Clear table content
    informations.clear();

    // Loop on all sections
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();

        // Abort if not expected table.
        if (sect.tableId() != _table_id) {
            return;
        }

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent();
        original_network_id = sect.tableIdExtension();

        // Loop across all informations.
        while (remain >= 5) {

            // Read fixed part.
            const uint16_t id = GetUInt16(data);
            Information& info(informations[id]);
            info.information_type = (data[2] >> 4) & 0x0F;
            info.description_body_location = (data[2] >> 2) & 0x03;
            info.user_defined = data[3];
            size_t count = data[4];
            data += 5; remain -= 5;

            // Read list of key ids.
            while (count > 0 && remain >= 2) {
                info.key_ids.push_back(GetUInt16(data));
                data += 2; remain -= 2; count--;
            }

            // Abort in case of truncated data.
            if (count != 0 || remain < 2) {
                return;
            }

            // Read list of descriptors.
            size_t length = GetUInt16(data) & 0x0FFF;
            data += 2; remain -= 2;
            length = std::min(length, remain);
            informations[id].descs.add(data, length);
            data += length; remain -= length;
        }

        // Abort in case of extra data in section.
        if (remain != 0) {
            return;
        }
    }

    _is_valid = true;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::NBIT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the sections
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    int section_number = 0;
    uint8_t* data = payload;
    size_t remain = sizeof(payload);

    // The section payload directly starts with the list of information sets.
    for (auto it = informations.begin(); it != informations.end(); ++it) {

        // If we are not at the beginning of the information loop, make sure that the entire
        // information set fits in the section. If it does not fit, start a new section. Huge
        // descriptions may not fit into one section, even when starting at the beginning
        // of the information loop. In that case, the information will span two sections later.
        const DescriptorList& dlist(it->second.descs);
        if (data > payload && 5 + 2 * it->second.key_ids.size() + 2 + dlist.binarySize() > remain) {
            // Create a new section
            addSection(table, section_number, payload, data, remain);
        }

        // Serialize the characteristics of the information set. Since the number of key ids is less
        // than 256 (must fit in one byte), the key id list will fit in the section. If the descriptor
        // list is too large to fit in one section, the key_id list won't be repeated in the next section.
        size_t key_count = std::min<size_t>(255, it->second.key_ids.size());

        for (size_t start_index = 0; ; ) {

            // Insert common characteristics (ie. ids).
            assert(remain >= 5);
            PutUInt16(data, it->first);
            PutUInt8(data + 2, uint8_t(it->second.information_type << 4) | uint8_t((it->second.description_body_location & 0x03) << 2) | 0x03);
            PutUInt8(data + 3, it->second.user_defined);
            PutUInt8(data + 4, uint8_t(key_count));
            data += 5; remain -= 5;

            // Insert the key id list.
            for (size_t i = 0; i < key_count; ++i) {
                PutUInt16(data, it->second.key_ids[i]);
                data += 2; remain -= 2;
            }

            // Don't repeat the key id list if we overflow the descriptor list in another section.
            key_count = 0;

            // Insert descriptors (all or some).
            start_index = dlist.lengthSerialize(data, remain, start_index);

            // Exit loop when all descriptors were serialized.
            if (start_index >= dlist.count()) {
                break;
            }

            // Not all descriptors were written, the section is full.
            // Open a new one and continue with this transport.
            addSection(table, section_number, payload, data, remain);
        }
    }

    // Add partial section.
    addSection(table, section_number, payload, data, remain);
}


//----------------------------------------------------------------------------
// Add a new section to a table being serialized.
//----------------------------------------------------------------------------

void ts::NBIT::addSection(BinaryTable& table, int& section_number, uint8_t* payload, uint8_t*& data, size_t& remain) const
{
    table.addSection(new Section(_table_id,
                                 true,                    // is_private_section
                                 original_network_id,     // ts id extension
                                 version,
                                 is_current,
                                 uint8_t(section_number),
                                 uint8_t(section_number), //last_section_number
                                 payload,
                                 data - payload));        // payload_size,

    // Reinitialize pointers after fixed part of payload (4 bytes).
    remain += data - payload;
    data = payload;
    section_number++;
}


//----------------------------------------------------------------------------
// A static method to display a NBIT section.
//----------------------------------------------------------------------------

void ts::NBIT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();

    strm << margin << UString::Format(u"Original network id: 0x%X (%d)", {section.tableIdExtension(), section.tableIdExtension()}) << std::endl;

    // Loop across all information sets.
    while (size >= 5) {
        strm << margin << UString::Format(u"- Information id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
             << margin << "  Information type: " << NameFromSection(u"ISDBInformationType", (data[2] >> 4) & 0x0F, names::FIRST) << std::endl
             << margin << "  Description body location: " << NameFromSection(u"ISDBDescriptionBodyLocation", (data[2] >> 2) & 0x03, names::FIRST) << std::endl
             << margin << UString::Format(u"  User defined: 0x%X (%d)", {data[3], data[3]}) << std::endl;
        size_t count = data[4];
        data += 5; size -= 5;

        while (count > 0 && size >= 2) {
            strm << margin << UString::Format(u"  Key id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl;
            data += 2; size -= 2; count--;
        }

        if (size >= 2) {
            size_t length = GetUInt16(data) & 0x0FFF;
            data += 2; size -= 2;
            length = std::min(length, size);
            display.displayDescriptorList(section, data, length, indent + 2);
            data += length; size -= length;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::NBIT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"original_network_id", original_network_id, true);
    root->setBoolAttribute(u"body", isBody());

    for (auto it = informations.begin(); it != informations.end(); ++it) {
        xml::Element* e = root->addElement(u"information");
        e->setIntAttribute(u"information_id", it->first, true);
        e->setIntAttribute(u"information_type", it->second.information_type, true);
        e->setIntAttribute(u"description_body_location", it->second.description_body_location, true);
        if (it->second.user_defined != 0xFF) {
            e->setIntAttribute(u"user_defined", it->second.user_defined, true);
        }
        for (size_t i = 0; i < it->second.key_ids.size(); ++i) {
            e->addElement(u"key")->setIntAttribute(u"id", it->second.key_ids[i], true);
        }
        it->second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::NBIT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xinfo;
    bool body = true;
    bool ok =
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute<uint16_t>(original_network_id, u"original_network_id", true) &&
        element->getBoolAttribute(body, u"body", false, true) &&
        element->getChildren(xinfo, u"information");

    if (body) {
        setBody();
    }
    else {
        setReference();
    }

    for (auto it1 = xinfo.begin(); ok && it1 != xinfo.end(); ++it1) {
        uint16_t id = 0;
        xml::ElementVector xkey;
        ok = (*it1)->getIntAttribute<uint16_t>(id, u"information_id", true) &&
             (*it1)->getIntAttribute<uint8_t>(informations[id].information_type, u"information_type", true, 0, 0, 15) &&
             (*it1)->getIntAttribute<uint8_t>(informations[id].description_body_location, u"description_body_location", true, 0, 0, 3) &&
             (*it1)->getIntAttribute<uint8_t>(informations[id].user_defined, u"user_defined", false, 0xFF) &&
             informations[id].descs.fromXML(duck, xkey, *it1, u"key");
        for (auto it2 = xkey.begin(); ok && it2 != xkey.end(); ++it2) {
            uint16_t kid = 0;
            ok = (*it2)->getIntAttribute<uint16_t>(kid, u"id", true);
            if (ok) {
                informations[id].key_ids.push_back(kid);
            }
        }
    }
    return ok;
}
