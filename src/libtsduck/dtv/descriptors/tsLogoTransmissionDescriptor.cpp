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

#include "tsLogoTransmissionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"logo_transmission_descriptor"
#define MY_CLASS ts::LogoTransmissionDescriptor
#define MY_DID ts::DID_ISDB_LOGO_TRANSM
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::LogoTransmissionDescriptor::LogoTransmissionDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    logo_transmission_type(0),
    logo_id(0),
    logo_version(0),
    download_data_id(0),
    logo_char(),
    reserved_future_use()
{
}

void ts::LogoTransmissionDescriptor::clearContent()
{
    logo_transmission_type = 0;
    logo_id = 0;
    logo_version = 0;
    download_data_id = 0;
    logo_char.clear();
    reserved_future_use.clear();
}

ts::LogoTransmissionDescriptor::LogoTransmissionDescriptor(DuckContext& duck, const Descriptor& desc) :
    LogoTransmissionDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::LogoTransmissionDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(logo_transmission_type);
    switch (logo_transmission_type) {
        case 0x01:
            bbp->appendUInt16(0xFE00 | logo_id);
            bbp->appendUInt16(0xF000 | logo_version);
            bbp->appendUInt16(download_data_id);
            break;
        case 0x02:
            bbp->appendUInt16(0xFE00 | logo_id);
            break;
        case 0x03:
            bbp->append(duck.encoded(logo_char));
            break;
        default:
            bbp->append(reserved_future_use);
            break;
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::LogoTransmissionDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1;

    logo_char.clear();
    reserved_future_use.clear();

    if (_is_valid) {
        logo_transmission_type = data[0];
        data += 1; size -= 1;
        switch (logo_transmission_type) {
            case 0x01:
                if (size != 6) {
                    _is_valid = false;
                }
                else {
                    logo_id = GetUInt16(data) & 0x01FF;
                    logo_version = GetUInt16(data + 2) & 0x0FFF;
                    download_data_id = GetUInt16(data + 4);
                }
                break;
            case 0x02:
                if (size != 2) {
                    _is_valid = false;
                }
                else {
                    logo_id = GetUInt16(data) & 0x01FF;
                }
                break;
            case 0x03:
                duck.decode(logo_char, data, size);
                break;
            default:
                reserved_future_use.copy(data, size);
                break;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::LogoTransmissionDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size > 0) {
        const uint8_t ttype = data[0];
        strm << margin << "Logo transmission type: " << NameFromSection(u"LogoTransmissionType", ttype, names::HEXA_FIRST) << std::endl;
        data += 1; size -= 1;

        switch (ttype) {
            case 0x01:
                if (size >= 6) {
                    const uint16_t id = GetUInt16(data) & 0x01FF;
                    const uint16_t version = GetUInt16(data + 2) & 0x0FFF;
                    const uint16_t ddid = GetUInt16(data + 4);
                    data += 6; size -= 6;
                    strm << margin << UString::Format(u"Logo id: 0x%03X (%d)", {id, id}) << std::endl
                         << margin << UString::Format(u"Logo version: 0x%03X (%d)", {version, version}) << std::endl
                         << margin << UString::Format(u"Download data id: 0x%X (%d)", {ddid, ddid}) << std::endl;
                }
                display.displayExtraData(data, size, indent);
                break;
            case 0x02:
                if (size >= 2) {
                    const uint16_t id = GetUInt16(data) & 0x01FF;
                    data += 2; size -= 2;
                    strm << margin << UString::Format(u"Logo id: 0x%03X (%d)", {id, id}) << std::endl;
                }
                display.displayExtraData(data, size, indent);
                break;
            case 0x03:
                strm << margin << "Logo characters: \"" << duck.decoded(data, size) << "\"" << std::endl;
                break;
            default:
                display.displayPrivateData(u"Reserved for future use", data, size, indent);
                break;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::LogoTransmissionDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"logo_transmission_type", logo_transmission_type, true);
    switch (logo_transmission_type) {
        case 0x01:
            root->setIntAttribute(u"logo_id", logo_id, true);
            root->setIntAttribute(u"logo_version", logo_version, true);
            root->setIntAttribute(u"download_data_id", download_data_id, true);
            break;
        case 0x02:
            root->setIntAttribute(u"logo_id", logo_id, true);
            break;
        case 0x03:
            root->setAttribute(u"logo_char", logo_char);
            break;
        default:
            root->addHexaTextChild(u"reserved_future_use", reserved_future_use, true);
            break;
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::LogoTransmissionDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute<uint8_t>(logo_transmission_type, u"logo_transmission_type", true) &&
           element->getIntAttribute<uint16_t>(logo_id, u"logo_id", logo_transmission_type == 0x01 || logo_transmission_type == 0x02, 0, 0, 0x01FF) &&
           element->getIntAttribute<uint16_t>(logo_version, u"logo_version", logo_transmission_type == 0x01, 0, 0, 0x0FFF) &&
           element->getIntAttribute<uint16_t>(download_data_id, u"download_data_id", logo_transmission_type == 0x01) &&
           element->getAttribute(logo_char, u"logo_char", logo_transmission_type == 0x03) &&
           element->getHexaTextChild(reserved_future_use, u"reserved_future_use");
}
