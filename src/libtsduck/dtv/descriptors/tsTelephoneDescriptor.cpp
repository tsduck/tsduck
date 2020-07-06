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

#include "tsTelephoneDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsDVBCharTableSingleByte.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"telephone_descriptor"
#define MY_CLASS ts::TelephoneDescriptor
#define MY_DID ts::DID_TELEPHONE
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TelephoneDescriptor::TelephoneDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    foreign_availability(false),
    connection_type(0),
    country_prefix(),
    international_area_code(),
    operator_code(),
    national_area_code(),
    core_number()
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

void ts::TelephoneDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
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
        desc.invalidate();
        return;
    }

    // Now we can safely serialize.
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8((foreign_availability ? 0xE0 : 0xC0) |
                     (connection_type & 0x1F));
    bbp->appendUInt8(0x80 |
                     uint8_t((bb_country_prefix.size() & 0x03) << 5) |
                     uint8_t((bb_international_area_code.size() & 0x07) << 2) |
                     uint8_t(bb_operator_code.size() & 0x03));
    bbp->appendUInt8(0x80 |
                     uint8_t((bb_national_area_code.size() & 0x07) << 4) |
                     uint8_t(bb_core_number.size() & 0x0F));
    bbp->append(bb_country_prefix);
    bbp->append(bb_international_area_code);
    bbp->append(bb_operator_code);
    bbp->append(bb_national_area_code);
    bbp->append(bb_core_number);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TelephoneDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 3;

    if (_is_valid) {
        foreign_availability = (data[0] & 0x20) != 0;
        connection_type = data[0] & 0x1F;
        const size_t country_len = (data[1] >> 5) & 0x03;
        const size_t inter_len = (data[1] >> 2) & 0x07;
        const size_t oper_len = data[1] & 0x03;
        const size_t nat_len = (data[2] >> 4) & 0x07;
        const size_t core_len = data[2] & 0x0F;
        data += 3; size -= 3;

        // ETSI EN 300 468 says that encoding shall be done using ISO/IEC 8859-1.
        _is_valid =
            size == country_len + inter_len + oper_len + nat_len + core_len &&
            DVBCharTableSingleByte::RAW_ISO_8859_1.decode(country_prefix, data, country_len) &&
            DVBCharTableSingleByte::RAW_ISO_8859_1.decode(international_area_code, data + country_len, inter_len) &&
            DVBCharTableSingleByte::RAW_ISO_8859_1.decode(operator_code, data + country_len + inter_len, oper_len) &&
            DVBCharTableSingleByte::RAW_ISO_8859_1.decode(national_area_code, data + country_len + inter_len + oper_len, nat_len) &&
            DVBCharTableSingleByte::RAW_ISO_8859_1.decode(core_number, data + country_len + inter_len + oper_len + nat_len, core_len);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TelephoneDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 3) {
        const uint8_t ctype = data[0] & 0x1F;
        strm << margin << UString::Format(u"Foreign availability: %s", {(data[0] & 0x20) != 0}) << std::endl
             << margin << UString::Format(u"Connection type: 0x%X (%d)", {ctype, ctype}) << std::endl;
        const size_t country_len = (data[1] >> 5) & 0x03;
        const size_t inter_len = (data[1] >> 2) & 0x07;
        const size_t oper_len = data[1] & 0x03;
        const size_t nat_len = (data[2] >> 4) & 0x07;
        const size_t core_len = data[2] & 0x0F;
        data += 3; size -= 3;

        UString str;
        if (size >= country_len && DVBCharTableSingleByte::RAW_ISO_8859_1.decode(str, data, country_len)) {
            data += country_len; size -= country_len;
            strm << margin << "Country prefix: \"" << str << "\"" << std::endl;

            if (size >= inter_len && DVBCharTableSingleByte::RAW_ISO_8859_1.decode(str, data, inter_len)) {
                data += inter_len; size -= inter_len;
                strm << margin << "International area code: \"" << str << "\"" << std::endl;

                if (size >= oper_len && DVBCharTableSingleByte::RAW_ISO_8859_1.decode(str, data, oper_len)) {
                    data += oper_len; size -= oper_len;
                    strm << margin << "Operator code: \"" << str << "\"" << std::endl;

                    if (size >= nat_len && DVBCharTableSingleByte::RAW_ISO_8859_1.decode(str, data, nat_len)) {
                        data += nat_len; size -= nat_len;
                        strm << margin << "National area code: \"" << str << "\"" << std::endl;

                        if (size >= core_len && DVBCharTableSingleByte::RAW_ISO_8859_1.decode(str, data, core_len)) {
                            data += core_len; size -= core_len;
                            strm << margin << "Core number: \"" << str << "\"" << std::endl;
                        }
                    }
                }
            }
        }
    }

    display.displayExtraData(data, size, indent);
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
            element->getIntAttribute<uint8_t>(connection_type, u"connection_type", true, 0, 0x00, 0x1F) &&
            element->getAttribute(country_prefix, u"country_prefix", false, UString(), 0, MAX_COUNTRY_PREFIX_LENGTH) &&
            element->getAttribute(international_area_code, u"international_area_code", false, UString(), 0, MAX_INTERNATIONAL_AREA_CODE_LENGTH) &&
            element->getAttribute(operator_code, u"operator_code", false, UString(), 0, MAX_OPERATOR_CODE_LENGTH) &&
            element->getAttribute(national_area_code, u"national_area_code", false, UString(), 0, MAX_NATIONAL_AREA_CODE_LENGTH) &&
            element->getAttribute(core_number, u"core_number", false, UString(), 0, MAX_CORE_NUMBER_LENGTH);
}
