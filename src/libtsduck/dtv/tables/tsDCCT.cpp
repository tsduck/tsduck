//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDCCT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    dcc_id(id),
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
    return uint16_t((uint16_t(dcc_subtype) << 8) | dcc_id);
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

void ts::DCCT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    dcc_subtype = uint8_t(section.tableIdExtension() >> 8);
    dcc_id = uint8_t(section.tableIdExtension());
    protocol_version = buf.getUInt8();

    // Loop on all upper-level definitions.
    uint8_t dcc_test_count = buf.getUInt8();
    while (!buf.error() && dcc_test_count-- > 0) {

        // Add a new Test at the end of the list.
        Test& test(tests.newEntry());

        test.dcc_context = DCCContext(buf.getBit());
        buf.skipBits(3);
        buf.getBits(test.dcc_from_major_channel_number, 10);
        buf.getBits(test.dcc_from_minor_channel_number, 10);
        buf.skipBits(4);
        buf.getBits(test.dcc_to_major_channel_number, 10);
        buf.getBits(test.dcc_to_minor_channel_number, 10);
        test.dcc_start_time = Time::GPSSecondsToUTC(buf.getUInt32());
        test.dcc_end_time = Time::GPSSecondsToUTC(buf.getUInt32());

        // Loop on all inner-level definitions.
        size_t dcc_term_count = buf.getUInt8();
        while (!buf.error() && dcc_term_count-- > 0) {

            // Add a new Term at the end of the list.
            Term& term(test.terms.newEntry());

            term.dcc_selection_type = buf.getUInt8();
            term.dcc_selection_id = buf.getUInt64();
            buf.getDescriptorListWithLength(term.descs, 10); // 10-bit length field
        }

        // Deserialize descriptor list for this DCC test.
        buf.getDescriptorListWithLength(test.descs, 10); // 10-bit length field
    }

    // Get descriptor list for the global table.
    buf.getDescriptorListWithLength(descs, 10); // 10-bit length field
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DCCT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // A DCCT is not allowed to use more than one section, see A/65, section 6.2.
    if (tests.size() > 255) {
        buf.setUserError();
        return;
    }

    buf.putUInt8(protocol_version);
    buf.putUInt8(uint8_t(tests.size()));

    // Add description of all DCC tests.
    for (const auto& it1 : tests) {
        const Test& test(it1.second);
        buf.putBit(test.dcc_context);
        buf.putBits(0xFF, 3);
        buf.putBits(test.dcc_from_major_channel_number, 10);
        buf.putBits(test.dcc_from_minor_channel_number, 10);
        buf.putBits(0xFF, 4);
        buf.putBits(test.dcc_to_major_channel_number, 10);
        buf.putBits(test.dcc_to_minor_channel_number, 10);
        buf.putUInt32(uint32_t(test.dcc_start_time.toGPSSeconds()));
        buf.putUInt32(uint32_t(test.dcc_end_time.toGPSSeconds()));

        if (test.terms.size() > 255) {
            buf.setUserError();
            return;
        }
        buf.putUInt8(uint8_t(test.terms.size()));

        // Add description of all DCC terms in this DCC test.
        for (const auto& it2 : test.terms) {
            const Term& term(it2.second);
            buf.putUInt8(term.dcc_selection_type);
            buf.putUInt64(term.dcc_selection_id);
            buf.putDescriptorListWithLength(term.descs, 0, NPOS, 10);
        }

        // Insert descriptor list for this DCC test (with leading 10-bit length field)
        buf.putDescriptorListWithLength(test.descs, 0, NPOS, 10);
    }

    // Insert common descriptor list (with leading 10-bit length field)
    buf.putDescriptorListWithLength(descs, 0, NPOS, 10);
}


//----------------------------------------------------------------------------
// A static method to display a DCCT section.
//----------------------------------------------------------------------------

void ts::DCCT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"DCC subtype: 0x%02X (%<d), DCC id: 0x%02X (%<d)", {section.tableIdExtension() >> 8, section.tableIdExtension() & 0xFF}) << std::endl;

    uint16_t dcc_test_count = 0;

    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"Protocol version: %d", {buf.getUInt8()});
        disp << UString::Format(u", number of DCC tests: %d", {dcc_test_count = buf.getUInt8()}) << std::endl;

        // Loop on all upper-level definitions.
        while (buf.canReadBytes(15) && dcc_test_count-- > 0) {

            const uint8_t ctx = buf.getBit();
            disp << margin << UString::Format(u"- DCC context: %d (%s)", {ctx, DCCContextNames.name(ctx)}) << std::endl;
            buf.skipBits(3);
            disp << margin << "  DCC from channel " << buf.getBits<uint16_t>(10);
            disp << "." << buf.getBits<uint16_t>(10);
            buf.skipBits(4);
            disp << " to channel " << buf.getBits<uint16_t>(10);
            disp << "." << buf.getBits<uint16_t>(10) << std::endl;
            disp << margin << "  Start UTC: " << Time::GPSSecondsToUTC(buf.getUInt32()).format(Time::DATETIME) << std::endl;
            disp << margin << "  End UTC:   " << Time::GPSSecondsToUTC(buf.getUInt32()).format(Time::DATETIME) << std::endl;

            size_t dcc_term_count = buf.getUInt8();
            disp << margin << "  Number of DCC selection terms: " << dcc_term_count << std::endl;

            // Loop on all DCC selection terms.
            while (dcc_term_count-- > 0 && buf.canReadBytes(9)) {
                disp << margin << "  - DCC selection type: " << DataName(MY_XML_NAME, u"selection_type", buf.getUInt8(), NamesFlags::FIRST) << std::endl;
                disp << margin << UString::Format(u"    DCC selection id: 0x%X", {buf.getUInt64()}) << std::endl;
                disp.displayDescriptorListWithLength(section, buf, margin + u"    ", u"DCC selection term descriptors:", UString(), 10);
            }

            // Display descriptor list for this DCC test.
            disp.displayDescriptorListWithLength(section, buf, margin + u"  ", u"DCC test descriptors:", UString(), 10);
        }

        // Display descriptor list for the global table.
        disp.displayDescriptorListWithLength(section, buf, margin, u"Additional descriptors:", UString(), 10);
    }
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

    for (const auto& it1 : tests) {
        const Test& test(it1.second);
        xml::Element* e1 = root->addElement(u"dcc_test");
        e1->setEnumAttribute(DCCContextNames, u"dcc_context", test.dcc_context);
        e1->setIntAttribute(u"dcc_from_major_channel_number", test.dcc_from_major_channel_number);
        e1->setIntAttribute(u"dcc_from_minor_channel_number", test.dcc_from_minor_channel_number);
        e1->setIntAttribute(u"dcc_to_major_channel_number", test.dcc_to_major_channel_number);
        e1->setIntAttribute(u"dcc_to_minor_channel_number", test.dcc_to_minor_channel_number);
        e1->setDateTimeAttribute(u"dcc_start_time", test.dcc_start_time);
        e1->setDateTimeAttribute(u"dcc_end_time", test.dcc_end_time);
        test.descs.toXML(duck, e1);

        for (const auto& it2 : test.terms) {
            const Term& term(it2.second);
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
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getIntAttribute(protocol_version, u"protocol_version", false, 0) &&
        element->getIntAttribute(dcc_subtype, u"dcc_subtype", false, 0) &&
        element->getIntAttribute(dcc_id, u"dcc_id", false, 0) &&
        descs.fromXML(duck, xtests, element, u"dcc_test");

    for (size_t i1 = 0; ok && i1 < xtests.size(); ++i1) {
        const xml::Element* const e1 = xtests[i1];
        xml::ElementVector xterms;
        Test& test(tests.newEntry()); // add a new Test at the end of the list.
        ok = e1->getIntEnumAttribute(test.dcc_context, DCCContextNames, u"dcc_context", true) &&
            e1->getIntAttribute(test.dcc_from_major_channel_number, u"dcc_from_major_channel_number", true) &&
            e1->getIntAttribute(test.dcc_from_minor_channel_number, u"dcc_from_minor_channel_number", true) &&
            e1->getIntAttribute(test.dcc_to_major_channel_number, u"dcc_to_major_channel_number", true) &&
            e1->getIntAttribute(test.dcc_to_minor_channel_number, u"dcc_to_minor_channel_number", true) &&
            e1->getDateTimeAttribute(test.dcc_start_time, u"dcc_start_time", true) &&
            e1->getDateTimeAttribute(test.dcc_end_time, u"dcc_end_time", true) &&
            test.descs.fromXML(duck, xterms, e1, u"dcc_term");

        for (size_t i2 = 0; ok && i2 < xterms.size(); ++i2) {
            const xml::Element* const e2 = xterms[i2];
            Term& term(test.terms.newEntry()); // add a new Term at the end of the list.
            ok = e2->getIntAttribute(term.dcc_selection_type, u"dcc_selection_type", true) &&
                 e2->getIntAttribute(term.dcc_selection_id, u"dcc_selection_id", true) &&
                 term.descs.fromXML(duck, e2);
        }
    }
    return ok;
}
