//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsLogoTransmissionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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

void ts::LogoTransmissionDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(logo_transmission_type);
    switch (logo_transmission_type) {
        case 0x01:
            buf.putBits(0xFF, 7);
            buf.putBits(logo_id, 9);
            buf.putBits(0xFF, 4);
            buf.putBits(logo_version, 12);
            buf.putUInt16(download_data_id);
            break;
        case 0x02:
            buf.putBits(0xFF, 7);
            buf.putBits(logo_id, 9);
            break;
        case 0x03:
            buf.putString(logo_char);
            break;
        default:
            buf.putBytes(reserved_future_use);
            break;
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::LogoTransmissionDescriptor::deserializePayload(PSIBuffer& buf)
{
    logo_transmission_type = buf.getUInt8();
    switch (logo_transmission_type) {
        case 0x01:
            buf.skipBits(7);
            buf.getBits(logo_id, 9);
            buf.skipBits(4);
            buf.getBits(logo_version, 12);
            download_data_id = buf.getUInt16();
            break;
        case 0x02:
            buf.skipBits(7);
            buf.getBits(logo_id, 9);
            break;
        case 0x03:
            buf.getString(logo_char);
            break;
        default:
            buf.getBytes(reserved_future_use);
            break;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::LogoTransmissionDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        const uint8_t ttype = buf.getUInt8();
        disp << margin << "Logo transmission type: " << DataName(MY_XML_NAME, u"Type", ttype, NamesFlags::HEXA_FIRST) << std::endl;
        if (ttype == 0x01 && buf.canReadBytes(6)) {
            buf.skipBits(7);
            disp << margin << UString::Format(u"Logo id: 0x%03X (%<d)", {buf.getBits<uint16_t>(9)}) << std::endl;
            buf.skipBits(4);
            disp << margin << UString::Format(u"Logo version: 0x%03X (%<d)", {buf.getBits<uint16_t>(12)}) << std::endl;
            disp << margin << UString::Format(u"Download data id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        }
        else if (ttype == 0x02 && buf.canReadBytes(2)) {
            buf.skipBits(7);
            disp << margin << UString::Format(u"Logo id: 0x%03X (%<d)", {buf.getBits<uint16_t>(9)}) << std::endl;
        }
        else if (ttype == 0x03) {
            disp << margin << "Logo characters: \"" << buf.getString() << "\"" << std::endl;
        }
        else {
            disp.displayPrivateData(u"Reserved for future use", buf, NPOS, margin);
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
    return element->getIntAttribute(logo_transmission_type, u"logo_transmission_type", true) &&
           element->getIntAttribute(logo_id, u"logo_id", logo_transmission_type == 0x01 || logo_transmission_type == 0x02, 0, 0, 0x01FF) &&
           element->getIntAttribute(logo_version, u"logo_version", logo_transmission_type == 0x01, 0, 0, 0x0FFF) &&
           element->getIntAttribute(download_data_id, u"download_data_id", logo_transmission_type == 0x01) &&
           element->getAttribute(logo_char, u"logo_char", logo_transmission_type == 0x03) &&
           element->getHexaTextChild(reserved_future_use, u"reserved_future_use");
}
