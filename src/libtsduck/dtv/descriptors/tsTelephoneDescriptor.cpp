//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTelephoneDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsDVBCharTableSingleByte.h"

#define MY_XML_NAME u"telephone_descriptor"
#define MY_CLASS ts::TelephoneDescriptor
#define MY_DID ts::DID_TELEPHONE
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TelephoneDescriptor::TelephoneDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::TelephoneDescriptor::TelephoneDescriptor(DuckContext& duck, const Descriptor& desc) :
    TelephoneDescriptor()
{
    deserialize(duck, desc);
}

void ts::TelephoneDescriptor::clearContent()
{
    foreign_availability = false;
    connection_type = 0;
    country_prefix.clear();
    international_area_code.clear();
    operator_code.clear();
    national_area_code.clear();
    core_number.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TelephoneDescriptor::serializePayload(PSIBuffer& buf) const
{
    // ETSI EN 300 468 says that encoding shall be done using ISO/IEC 8859-1.
    const ByteBlock bb_country_prefix(DVBCharTableSingleByte::RAW_ISO_8859_1.encoded(country_prefix));
    const ByteBlock bb_international_area_code(DVBCharTableSingleByte::RAW_ISO_8859_1.encoded(international_area_code));
    const ByteBlock bb_operator_code(DVBCharTableSingleByte::RAW_ISO_8859_1.encoded(operator_code));
    const ByteBlock bb_national_area_code(DVBCharTableSingleByte::RAW_ISO_8859_1.encoded(national_area_code));
    const ByteBlock bb_core_number(DVBCharTableSingleByte::RAW_ISO_8859_1.encoded(core_number));

    // Check that all string length match constraints.
    if (bb_country_prefix.size() > MAX_COUNTRY_PREFIX_LENGTH ||
        bb_international_area_code.size() > MAX_INTERNATIONAL_AREA_CODE_LENGTH ||
        bb_operator_code.size() > MAX_OPERATOR_CODE_LENGTH ||
        bb_national_area_code.size() > MAX_NATIONAL_AREA_CODE_LENGTH ||
        bb_core_number.size() > MAX_CORE_NUMBER_LENGTH)
    {
        buf.setUserError();
    }
    else {
        buf.putBits(0xFF, 2);
        buf.putBit(foreign_availability);
        buf.putBits(connection_type, 5);
        buf.putBit(1);
        buf.putBits(bb_country_prefix.size(), 2);
        buf.putBits(bb_international_area_code.size(), 3);
        buf.putBits(bb_operator_code.size(), 2);
        buf.putBit(1);
        buf.putBits(bb_national_area_code.size(), 3);
        buf.putBits(bb_core_number.size(), 4);
        buf.putBytes(bb_country_prefix);
        buf.putBytes(bb_international_area_code);
        buf.putBytes(bb_operator_code);
        buf.putBytes(bb_national_area_code);
        buf.putBytes(bb_core_number);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TelephoneDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(2);
    foreign_availability = buf.getBool();
    buf.getBits(connection_type, 5);
    buf.skipBits(1);
    const size_t country_len = buf.getBits<size_t>(2);
    const size_t inter_len = buf.getBits<size_t>(3);
    const size_t oper_len = buf.getBits<size_t>(2);
    buf.skipBits(1);
    const size_t nat_len = buf.getBits<size_t>(3);
    const size_t core_len = buf.getBits<size_t>(4);
    buf.getString(country_prefix, country_len, &DVBCharTableSingleByte::RAW_ISO_8859_1);
    buf.getString(international_area_code, inter_len, &DVBCharTableSingleByte::RAW_ISO_8859_1);
    buf.getString(operator_code, oper_len, &DVBCharTableSingleByte::RAW_ISO_8859_1);
    buf.getString(national_area_code, nat_len, &DVBCharTableSingleByte::RAW_ISO_8859_1);
    buf.getString(core_number, core_len, &DVBCharTableSingleByte::RAW_ISO_8859_1);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TelephoneDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        buf.skipBits(2);
        disp << margin << UString::Format(u"Foreign availability: %s", {buf.getBool()}) << std::endl;
        disp << margin << UString::Format(u"Connection type: 0x%X (%<d)", {buf.getBits<uint8_t>(5)}) << std::endl;
        buf.skipBits(1);
        const size_t country_len = buf.getBits<size_t>(2);
        const size_t inter_len = buf.getBits<size_t>(3);
        const size_t oper_len = buf.getBits<size_t>(2);
        buf.skipBits(1);
        const size_t nat_len = buf.getBits<size_t>(3);
        const size_t core_len = buf.getBits<size_t>(4);
        disp << margin << "Country prefix: \"" << buf.getString(country_len, &DVBCharTableSingleByte::RAW_ISO_8859_1) << "\"" << std::endl;
        disp << margin << "International area code: \"" << buf.getString(inter_len, &DVBCharTableSingleByte::RAW_ISO_8859_1) << "\"" << std::endl;
        disp << margin << "Operator code: \"" << buf.getString(oper_len, &DVBCharTableSingleByte::RAW_ISO_8859_1) << "\"" << std::endl;
        disp << margin << "National area code: \"" << buf.getString(nat_len, &DVBCharTableSingleByte::RAW_ISO_8859_1) << "\"" << std::endl;
        disp << margin << "Core number: \"" << buf.getString(core_len, &DVBCharTableSingleByte::RAW_ISO_8859_1) << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TelephoneDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"foreign_availability", foreign_availability);
    root->setIntAttribute(u"connection_type", connection_type);
    root->setAttribute(u"country_prefix", country_prefix, true);
    root->setAttribute(u"international_area_code", international_area_code, true);
    root->setAttribute(u"operator_code", operator_code, true);
    root->setAttribute(u"national_area_code", national_area_code, true);
    root->setAttribute(u"core_number", core_number, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TelephoneDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return  element->getBoolAttribute(foreign_availability, u"foreign_availability", true) &&
            element->getIntAttribute(connection_type, u"connection_type", true, 0, 0x00, 0x1F) &&
            element->getAttribute(country_prefix, u"country_prefix", false, UString(), 0, MAX_COUNTRY_PREFIX_LENGTH) &&
            element->getAttribute(international_area_code, u"international_area_code", false, UString(), 0, MAX_INTERNATIONAL_AREA_CODE_LENGTH) &&
            element->getAttribute(operator_code, u"operator_code", false, UString(), 0, MAX_OPERATOR_CODE_LENGTH) &&
            element->getAttribute(national_area_code, u"national_area_code", false, UString(), 0, MAX_NATIONAL_AREA_CODE_LENGTH) &&
            element->getAttribute(core_number, u"core_number", false, UString(), 0, MAX_CORE_NUMBER_LENGTH);
}
