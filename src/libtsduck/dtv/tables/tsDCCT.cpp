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

#include "tsDCCT.h"
#include "tsNames.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"DCCT"
#define MY_CLASS ts::DCCT
#define MY_TID ts::TID_DCCT
#define MY_STD ts::Standards::ATSC

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection);

const ts::Enumeration ts::DCCT::DCCContextNames({
    {u"temporary_retune", ts::DCCT::temporary_retune},
    {u"channel_redirect", ts::DCCT::channel_redirect},
});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DCCT::DCCT(uint8_t vers, uint8_t id) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, vers, true), // DCCT is always "current"
    dcc_subtype(0),
    dcc_id(id),
    protocol_version(0),
    tests(this),
    descs(this)
{
}

ts::DCCT::DCCT(const DCCT& other) :
    AbstractLongTable(other),
    dcc_subtype(other.dcc_subtype),
    dcc_id(other.dcc_id),
    protocol_version(other.protocol_version),
    tests(this, other.tests),
    descs(this, other.descs)
{
}

ts::DCCT::DCCT(DuckContext& duck, const BinaryTable& table) :
    DCCT()
{
    deserialize(duck, table);
}

ts::DCCT::Test::Test(const AbstractTable* table, DCCContext ctx) :
    EntryWithDescriptors(table),
    dcc_context(ctx),
    dcc_from_major_channel_number(0),
    dcc_from_minor_channel_number(0),
    dcc_to_major_channel_number(0),
    dcc_to_minor_channel_number(0),
    dcc_start_time(),
    dcc_end_time(),
    terms(table)
{
}

ts::DCCT::Term::Term(const AbstractTable* table, uint8_t type, uint64_t id) :
    EntryWithDescriptors(table),
    dcc_selection_type(type),
    dcc_selection_id(id)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::DCCT::tableIdExtension() const
{
    return  uint16_t((uint16_t(dcc_subtype) << 8) | dcc_id);
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::DCCT::clearContent()
{
    dcc_subtype = 0;
    dcc_id = 0;
    protocol_version = 0;
    descs.clear();
    tests.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DCCT::deserializeContent(DuckContext& duck, const BinaryTable& table)
{
    clear();

    // Loop on all sections (although a DCCT is not allowed to use more than one section, see A/65, section 6.7)
    for (size_t si = 0; si < table.sectionCount(); ++si) {

        // Reference to current section
        const Section& sect(*table.sectionAt(si));

        // Get common properties (should be identical in all sections)
        version = sect.version();
        is_current = sect.isCurrent(); // should be true
        dcc_subtype = uint8_t(sect.tableIdExtension() >> 8);
        dcc_id = uint8_t(sect.tableIdExtension());

        // Analyze the section payload:
        const uint8_t* data = sect.payload();
        size_t remain = sect.payloadSize();
        if (remain < 2) {
            return; // invalid table, too short
        }

        // Get fixed fields.
        protocol_version = data[0];
        uint8_t dcc_test_count = data[1];
        data += 2; remain -= 2;

        // Loop on all DCC test definitions.
        while (dcc_test_count > 0 && remain >= 15) {

            // Add a new Test at the end of the list.
            Test& test(tests.newEntry());

            const uint32_t from = GetUInt24(data);
            const uint32_t to = GetUInt24(data + 3);
            test.dcc_context = DCCContext((data[0] >> 7) & 0x01);
            test.dcc_from_major_channel_number = uint16_t((from >> 10) & 0x03FF);
            test.dcc_from_minor_channel_number = uint16_t(from & 0x03FF);
            test.dcc_to_major_channel_number = uint16_t((to >> 10) & 0x03FF);
            test.dcc_to_minor_channel_number = uint16_t(to & 0x03FF);
            test.dcc_start_time = Time::GPSSecondsToUTC(GetUInt32(data + 6));
            test.dcc_end_time = Time::GPSSecondsToUTC(GetUInt32(data + 10));
            size_t dcc_term_count = data[14];
            data += 15; remain -= 15;

            // Deserialize all DCC terms in this DCC test.
            while (dcc_term_count > 0 && remain >= 11) {

                // Add a new Term at the end of the list.
                Term& term(test.terms.newEntry());

                term.dcc_selection_type = data[0];
                term.dcc_selection_id = GetUInt64(data + 1);
                size_t len = GetUInt16(data + 9) & 0x03FF;
                data += 11; remain -= 11;

                len = std::min(len, remain);
                term.descs.add(data, len);
                data += len; remain -= len;

                dcc_term_count--;
            }

            // Deserialize descriptor list for this DCC test.
            if (remain < 2) {
                return; // invalid table, too short
            }
            size_t len = GetUInt16(data) & 0x03FF;
            data += 2; remain -= 2;
            len = std::min(len, remain);
            test.descs.add(data, len);
            data += len; remain -= len;

            dcc_test_count--;
        }
        if (dcc_test_count > 0 || remain < 2) {
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

void ts::DCCT::serializeContent(DuckContext& duck, BinaryTable& table) const
{
    // Build the section. Note that a DCCT is not allowed to use more than one section, see A/65, section 6.2.
    uint8_t payload[MAX_PRIVATE_LONG_SECTION_PAYLOAD_SIZE];
    uint8_t* data = payload;
    size_t remain = sizeof(payload);

    // Add fixed fields.
    data[0] = protocol_version;
    data[1] = uint8_t(tests.size());
    data += 2; remain -= 2;

    // Add description of all DCC tests.
    for (auto it1 = tests.begin(); it1 != tests.end() && remain >= 15; ++it1) {
        const Test& test(it1->second);

        // Insert fixed fields.
        PutUInt24(data, (uint32_t(test.dcc_context & 0x01) << 23) |
                        (uint32_t(test.dcc_from_major_channel_number & 0x03FF) << 10) |
                        uint32_t(test.dcc_from_minor_channel_number & 0x03FF));
        PutUInt24(data + 3, 0x00F00000 |
                            (uint32_t(test.dcc_to_major_channel_number & 0x03FF) << 10) |
                            uint32_t(test.dcc_to_minor_channel_number & 0x03FF));
        PutUInt32(data + 6, uint32_t(test.dcc_start_time.toGPSSeconds()));
        PutUInt32(data + 10, uint32_t(test.dcc_end_time.toGPSSeconds()));
        PutUInt8(data + 14, uint8_t(test.terms.size()));
        data += 15; remain -= 15;

        // Add description of all DCC terms in this DCC test.
        for (auto it2 = test.terms.begin(); it2 != test.terms.end() && remain >= 9; ++it2) {
            const Term& term(it2->second);
            PutUInt8(data, term.dcc_selection_type);
            PutUInt64(data + 1, term.dcc_selection_id);
            data += 9; remain -= 9;

            // Insert descriptor list for this DCC term (with leading length field)
            size_t next_index = term.descs.lengthSerialize(data, remain, 0, 0x003F, 10);
            if (next_index < term.descs.count()) {
                // Not enough space to serialize all descriptors in the section.
                return;
            }

        }

        // Insert descriptor list for this DCC test (with leading length field)
        size_t next_index = test.descs.lengthSerialize(data, remain, 0, 0x003F, 10);
        if (next_index < test.descs.count()) {
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
    table.addSection(new Section(MY_TID,             // tid
                                 true,               // is_private_section
                                 tableIdExtension(), // tid_ext
                                 version,
                                 is_current,         // should be true
                                 0,                  // section_number,
                                 0,                  // last_section_number
                                 payload,
                                 data - payload));   // payload_size,
}


//----------------------------------------------------------------------------
// A static method to display a DCCT section.
//----------------------------------------------------------------------------

void ts::DCCT::DisplaySection(TablesDisplay& display, const ts::Section& section, int indent)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    const uint8_t* data = section.payload();
    size_t size = section.payloadSize();
    bool ok = true;

    if (size >= 2) {
        // Fixed part.
        const uint8_t subtype = uint8_t(section.tableIdExtension() >> 8);
        const uint8_t id = uint8_t(section.tableIdExtension());
        size_t dcc_test_count = data[1];
        strm << margin << UString::Format(u"DCC subtype: 0x%X (%d), DCC id: 0x%X (%d)", {subtype, subtype, id, id}) << std::endl
             << margin << UString::Format(u"Protocol version: %d, number of DCC tests: %d", {data[0], dcc_test_count}) << std::endl;
        data += 2; size -= 2;

        // Loop on all DCC tests.
        while (ok && dcc_test_count > 0) {

            // Check fixed part.
            ok = size >= 15;
            if (!ok) {
                break;
            }

            // Display fixed part.
            const uint8_t ctx = (data[0] >> 7) & 0x01;
            const uint32_t from = GetUInt24(data);
            const uint32_t to = GetUInt24(data + 3);
            const Time start(Time::GPSSecondsToUTC(GetUInt32(data + 6)));
            const Time end(Time::GPSSecondsToUTC(GetUInt32(data + 10)));
            size_t dcc_term_count = data[14];
            data += 15; size -= 15;

            strm << margin << UString::Format(u"- DCC context: %d (%s)", {ctx, DCCContextNames.name(ctx)}) << std::endl
                 << margin << UString::Format(u"  DCC from channel %d.%d to channel %d.%d", {(from >> 10) & 0x03FF, from & 0x03FF, (to >> 10) & 0x03FF, to & 0x03FF}) << std::endl
                 << margin << "  Start UTC: " << start.format(Time::DATETIME) << std::endl
                 << margin << "  End UTC:   " << end.format(Time::DATETIME) << std::endl
                 << margin << "  Number of DCC selection terms: " << dcc_term_count << std::endl;

            // Loop on all DCC selection terms.
            while (ok && dcc_term_count > 0) {

                // Check fixed part.
                ok = size >= 9;
                if (!ok) {
                    break;
                }

                // Display fixed part.
                strm << margin << "  - DCC selection type: " << NameFromSection(u"DCCSelectionType", data[0], names::FIRST) << std::endl
                     << margin << UString::Format(u"    DCC selection id: 0x%X", {GetUInt64(data + 1)}) << std::endl;
                data += 9; size -= 9;

                // Display descriptor list for this term.
                ok = ok && size >= 2;
                if (ok) {
                    size_t len = GetUInt16(data) & 0x03FF;
                    data += 2; size -= 2;
                    len = std::min(len, size);
                    if (len > 0) {
                        strm << margin << "    DCC selection term descriptors:" << std::endl;
                        display.displayDescriptorList(section, data, len, indent + 4);
                        data += len; size -= len;
                    }
                }

                dcc_term_count--;
            }

            // Display descriptor list for this DCC test.
            ok = ok && size >= 2;
            if (ok) {
                size_t len = GetUInt16(data) & 0x03FF;
                data += 2; size -= 2;
                len = std::min(len, size);
                if (len > 0) {
                    strm << margin << "  DCC test descriptors:" << std::endl;
                    display.displayDescriptorList(section, data, len, indent + 2);
                    data += len; size -= len;
                }
            }

            dcc_test_count--;
        }

        // Display descriptor list for the global table.
        if (ok && size >= 2) {
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

void ts::DCCT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setIntAttribute(u"protocol_version", protocol_version);
    root->setIntAttribute(u"dcc_subtype", dcc_subtype, true);
    root->setIntAttribute(u"dcc_id", dcc_id, true);
    descs.toXML(duck, root);

    for (auto it1 = tests.begin(); it1 != tests.end(); ++it1) {
        const Test& test(it1->second);
        xml::Element* e1 = root->addElement(u"dcc_test");
        e1->setEnumAttribute(DCCContextNames, u"dcc_context", test.dcc_context);
        e1->setIntAttribute(u"dcc_from_major_channel_number", test.dcc_from_major_channel_number);
        e1->setIntAttribute(u"dcc_from_minor_channel_number", test.dcc_from_minor_channel_number);
        e1->setIntAttribute(u"dcc_to_major_channel_number", test.dcc_to_major_channel_number);
        e1->setIntAttribute(u"dcc_to_minor_channel_number", test.dcc_to_minor_channel_number);
        e1->setDateTimeAttribute(u"dcc_start_time", test.dcc_start_time);
        e1->setDateTimeAttribute(u"dcc_end_time", test.dcc_end_time);
        test.descs.toXML(duck, e1);

        for (auto it2 = test.terms.begin(); it2 != test.terms.end(); ++it2) {
            const Term& term(it2->second);
            xml::Element* e2 = e1->addElement(u"dcc_term");
            e2->setIntAttribute(u"dcc_selection_type", term.dcc_selection_type, true);
            e2->setIntAttribute(u"dcc_selection_id", term.dcc_selection_id, true);
            term.descs.toXML(duck, e2);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DCCT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xtests;
    bool ok =
        element->getIntAttribute<uint8_t>(version, u"version", false, 0, 0, 31) &&
        element->getIntAttribute<uint8_t>(protocol_version, u"protocol_version", false, 0) &&
        element->getIntAttribute<uint8_t>(dcc_subtype, u"dcc_subtype", false, 0) &&
        element->getIntAttribute<uint8_t>(dcc_id, u"dcc_id", false, 0) &&
        descs.fromXML(duck, xtests, element, u"dcc_test");

    for (size_t i1 = 0; ok && i1 < xtests.size(); ++i1) {
        const xml::Element* const e1 = xtests[i1];
        xml::ElementVector xterms;
        Test& test(tests.newEntry()); // add a new Test at the end of the list.
        ok = e1->getIntEnumAttribute(test.dcc_context, DCCContextNames, u"dcc_context", true) &&
            e1->getIntAttribute<uint16_t>(test.dcc_from_major_channel_number, u"dcc_from_major_channel_number", true) &&
            e1->getIntAttribute<uint16_t>(test.dcc_from_minor_channel_number, u"dcc_from_minor_channel_number", true) &&
            e1->getIntAttribute<uint16_t>(test.dcc_to_major_channel_number, u"dcc_to_major_channel_number", true) &&
            e1->getIntAttribute<uint16_t>(test.dcc_to_minor_channel_number, u"dcc_to_minor_channel_number", true) &&
            e1->getDateTimeAttribute(test.dcc_start_time, u"dcc_start_time", true) &&
            e1->getDateTimeAttribute(test.dcc_end_time, u"dcc_end_time", true) &&
            test.descs.fromXML(duck, xterms, e1, u"dcc_term");

        for (size_t i2 = 0; ok && i2 < xterms.size(); ++i2) {
            const xml::Element* const e2 = xterms[i2];
            Term& term(test.terms.newEntry()); // add a new Term at the end of the list.
            ok = e2->getIntAttribute<uint8_t>(term.dcc_selection_type, u"dcc_selection_type", true) &&
                 e2->getIntAttribute<uint64_t>(term.dcc_selection_id, u"dcc_selection_id", true) &&
                 term.descs.fromXML(duck, e2);
        }
    }
    return ok;
}
