//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMetadataDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"metadata_descriptor"
#define MY_CLASS ts::MetadataDescriptor
#define MY_DID ts::DID_METADATA
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MetadataDescriptor::MetadataDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::MetadataDescriptor::clearContent()
{
    metadata_application_format = 0;
    metadata_application_format_identifier = 0;
    metadata_format = 0;
    metadata_format_identifier = 0;
    metadata_service_id = 0;
    decoder_config_flags = 0;
    service_identification.clear();
    decoder_config.clear();
    dec_config_identification.clear();
    decoder_config_metadata_service_id = 0;
    reserved_data.clear();
    private_data.clear();
}

ts::MetadataDescriptor::MetadataDescriptor(DuckContext& duck, const Descriptor& desc) :
    MetadataDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization.
//----------------------------------------------------------------------------

void ts::MetadataDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(metadata_application_format);
    if (metadata_application_format == 0xFFFF) {
        buf.putUInt32(metadata_application_format_identifier);
    }
    buf.putUInt8(metadata_format);
    if (metadata_format == 0xFF) {
        buf.putUInt32(metadata_format_identifier);
    }
    buf.putUInt8(metadata_service_id);
    buf.putBits(decoder_config_flags, 3);
    buf.putBit(!service_identification.empty());
    buf.putBits(0xFF, 4);
    if (!service_identification.empty()) {
        buf.putUInt8(uint8_t(service_identification.size()));
        buf.putBytes(service_identification);
    }
    if (decoder_config_flags == 1) {
        buf.putUInt8(uint8_t(decoder_config.size()));
        buf.putBytes(decoder_config);
    }
    else if (decoder_config_flags == 3) {
        buf.putUInt8(uint8_t(dec_config_identification.size()));
        buf.putBytes(dec_config_identification);
    }
    else if (decoder_config_flags == 4) {
        buf.putUInt8(decoder_config_metadata_service_id);
    }
    else if (decoder_config_flags == 5 || decoder_config_flags == 6) {
        buf.putUInt8(uint8_t(reserved_data.size()));
        buf.putBytes(reserved_data);
    }
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization.
//----------------------------------------------------------------------------

void ts::MetadataDescriptor::deserializePayload(PSIBuffer& buf)
{
    metadata_application_format = buf.getUInt16();
    if (metadata_application_format == 0xFFFF) {
        metadata_application_format_identifier = buf.getUInt32();
    }
    metadata_format = buf.getUInt8();
    if (metadata_format == 0xFF) {
        metadata_format_identifier = buf.getUInt32();
    }
    metadata_service_id = buf.getUInt8();
    buf.getBits(decoder_config_flags, 3);
    const bool DSMCC_flag = buf.getBool();
    buf.skipBits(4);
    if (DSMCC_flag) {
        const size_t length = buf.getUInt8();
        buf.getBytes(service_identification, length);
    }
    if (decoder_config_flags == 1) {
        const size_t length = buf.getUInt8();
        buf.getBytes(decoder_config, length);
    }
    else if (decoder_config_flags == 3) {
        const size_t length = buf.getUInt8();
        buf.getBytes(dec_config_identification, length);
    }
    else if (decoder_config_flags == 4) {
        decoder_config_metadata_service_id = buf.getUInt8();
    }
    else if (decoder_config_flags == 5 || decoder_config_flags == 6) {
        const size_t length = buf.getUInt8();
        buf.getBytes(reserved_data, length);
    }
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MetadataDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (!buf.canReadBytes(2)) {
        buf.setUserError();
    }
    else {
        const uint16_t format = buf.getUInt16();
        disp << margin << "Metadata application format: " << DataName(MY_XML_NAME, u"application_format", format, NamesFlags::HEXA_FIRST) << std::endl;
        if (format == 0xFFFF && buf.remainingReadBytes() >= 4) {
            disp << margin << UString::Format(u"Metadata application format identifier: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;
        }
    }

    if (!buf.canReadBytes(1)) {
        buf.setUserError();
    }
    else {
        const uint8_t format = buf.getUInt8();
        disp << margin << "Metadata format: " << DataName(MY_XML_NAME, u"metadata_format", format, NamesFlags::HEXA_FIRST) << std::endl;
        if (format == 0xFF && buf.remainingReadBytes() >= 4) {
            disp << margin << UString::Format(u"Metadata format identifier: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;
        }
    }

    if (!buf.canReadBytes(2)) {
        buf.setUserError();
    }
    else {
        disp << margin << UString::Format(u"Metadata service id: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        const uint8_t flags = buf.getBits<uint8_t>(3);
        const bool DSMCC_flag = buf.getBool();
        buf.skipBits(4);
        disp << margin << "Decoder config flags: " << DataName(MY_XML_NAME, u"decoder_config_flags", flags, NamesFlags::DECIMAL_FIRST) << std::endl;
        if (DSMCC_flag) {
            const size_t length = buf.getUInt8();
            disp.displayPrivateData(u"Service identification record", buf, length, margin);
        }
        if (flags == 1) {
            const size_t length = buf.getUInt8();
            disp.displayPrivateData(u"Decoder config", buf, length, margin);
        }
        else if (flags == 3) {
            const size_t length = buf.getUInt8();
            disp.displayPrivateData(u"Decoder config identification record", buf, length, margin);
        }
        else if (flags == 4) {
            disp << margin << UString::Format(u"Decoder config metadata service id: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        }
        else if (flags == 5 || flags == 6) {
            const size_t length = buf.getUInt8();
            disp.displayPrivateData(u"Reserved data", buf, length, margin);
        }
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization.
//----------------------------------------------------------------------------

void ts::MetadataDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"metadata_application_format", metadata_application_format, true);
    if (metadata_application_format == 0xFFFF) {
        root->setIntAttribute(u"metadata_application_format_identifier", metadata_application_format_identifier, true);
    }
    root->setIntAttribute(u"metadata_format", metadata_format, true);
    if (metadata_format == 0xFF) {
        root->setIntAttribute(u"metadata_format_identifier", metadata_format_identifier, true);
    }
    root->setIntAttribute(u"metadata_service_id", metadata_service_id, true);
    root->setIntAttribute(u"decoder_config_flags", decoder_config_flags, false);
    if (decoder_config_flags == 4) {
        root->setIntAttribute(u"decoder_config_metadata_service_id", decoder_config_metadata_service_id, true);
    }
    root->addHexaTextChild(u"service_identification", service_identification, true);
    root->addHexaTextChild(u"decoder_config", decoder_config, true);
    root->addHexaTextChild(u"dec_config_identification", dec_config_identification, true);
    root->addHexaTextChild(u"reserved_data", reserved_data, true);
    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization.
//----------------------------------------------------------------------------

bool ts::MetadataDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(metadata_application_format, u"metadata_application_format", true) &&
           element->getIntAttribute(metadata_application_format_identifier, u"metadata_application_format_identifier", metadata_application_format == 0xFFFF) &&
           element->getIntAttribute(metadata_format, u"metadata_format", true) &&
           element->getIntAttribute(metadata_format_identifier, u"metadata_format_identifier", metadata_format == 0xFF) &&
           element->getIntAttribute(metadata_service_id, u"metadata_service_id", true) &&
           element->getIntAttribute(decoder_config_flags, u"decoder_config_flags", true, 0, 0, 15) &&
           element->getIntAttribute(decoder_config_metadata_service_id, u"decoder_config_metadata_service_id", decoder_config_flags == 4) &&
           element->getHexaTextChild(service_identification, u"service_identification", false, 0, 255) &&
           element->getHexaTextChild(decoder_config, u"decoder_config", false, 0, 255) &&
           element->getHexaTextChild(dec_config_identification, u"dec_config_identification", false, 0, 255) &&
           element->getHexaTextChild(reserved_data, u"reserved_data", false, 0, 255) &&
           element->getHexaTextChild(private_data, u"private_data", false, 0, 255);
}
