//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMetadataPointerDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"metadata_pointer_descriptor"
#define MY_CLASS ts::MetadataPointerDescriptor
#define MY_DID ts::DID_METADATA_POINTER
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MetadataPointerDescriptor::MetadataPointerDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::MetadataPointerDescriptor::clearContent()
{
    metadata_application_format = 0;
    metadata_application_format_identifier = 0;
    metadata_format = 0;
    metadata_format_identifier = 0;
    metadata_service_id = 0;
    MPEG_carriage_flags = 0;
    metadata_locator.clear();
    program_number = 0;
    transport_stream_location = 0;
    transport_stream_id = 0;
    private_data.clear();
}

ts::MetadataPointerDescriptor::MetadataPointerDescriptor(DuckContext& duck, const Descriptor& desc) :
    MetadataPointerDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization.
//----------------------------------------------------------------------------

void ts::MetadataPointerDescriptor::serializePayload(PSIBuffer& buf) const
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
    buf.putBit(!metadata_locator.empty());
    buf.putBits(MPEG_carriage_flags, 2);
    buf.putBits(0xFF, 5);
    if (!metadata_locator.empty()) {
        buf.putUInt8(uint8_t(metadata_locator.size()));
        buf.putBytes(metadata_locator);
    }
    if (MPEG_carriage_flags <= 2) {
        buf.putUInt16(program_number);
    }
    if (MPEG_carriage_flags == 1) {
        buf.putUInt16(transport_stream_location);
        buf.putUInt16(transport_stream_id);
    }
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization.
//----------------------------------------------------------------------------

void ts::MetadataPointerDescriptor::deserializePayload(PSIBuffer& buf)
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
    const bool metadata_locator_record_flag = buf.getBool();
    buf.getBits(MPEG_carriage_flags, 2);
    buf.skipBits(5);
    if (metadata_locator_record_flag) {
        const size_t length = buf.getUInt8();
        buf.getBytes(metadata_locator, length);
    }
    if (MPEG_carriage_flags <= 2) {
        program_number = buf.getUInt16();
    }
    if (MPEG_carriage_flags == 1) {
        transport_stream_location = buf.getUInt16();
        transport_stream_id = buf.getUInt16();
    }
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MetadataPointerDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
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
        const bool metadata_locator_record_flag = buf.getBool();
        const uint8_t flags = buf.getBits<uint8_t>(2);
        disp << margin << "MPEG carriage flags: " << DataName(MY_XML_NAME, u"carriage_flags", flags, NamesFlags::DECIMAL_FIRST) << std::endl;
        buf.skipBits(5);
        if (metadata_locator_record_flag) {
            const size_t length = buf.getUInt8();
            disp.displayPrivateData(u"Metadata location record", buf, length, margin);
        }
        if (flags <= 2 && buf.remainingReadBytes() >= 2) {
            disp << margin << UString::Format(u"Program number: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        }
        if (flags == 1 && buf.remainingReadBytes() >= 4) {
            disp << margin << UString::Format(u"Transport stream location: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            disp << margin << UString::Format(u"Transport stream id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        }
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization.
//----------------------------------------------------------------------------

void ts::MetadataPointerDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
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
    root->setIntAttribute(u"MPEG_carriage_flags", MPEG_carriage_flags, false);
    root->addHexaTextChild(u"metadata_locator", metadata_locator, true);
    if (MPEG_carriage_flags <= 2) {
        root->setIntAttribute(u"program_number", program_number, true);
    }
    if (MPEG_carriage_flags == 1) {
        root->setIntAttribute(u"transport_stream_location", transport_stream_location, true);
        root->setIntAttribute(u"transport_stream_id", transport_stream_id, true);
    }
    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization.
//----------------------------------------------------------------------------

bool ts::MetadataPointerDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(metadata_application_format, u"metadata_application_format", true) &&
           element->getIntAttribute(metadata_application_format_identifier, u"metadata_application_format_identifier", metadata_application_format == 0xFFFF) &&
           element->getIntAttribute(metadata_format, u"metadata_format", true) &&
           element->getIntAttribute(metadata_format_identifier, u"metadata_format_identifier", metadata_format == 0xFF) &&
           element->getIntAttribute(metadata_service_id, u"metadata_service_id", true) &&
           element->getIntAttribute(MPEG_carriage_flags, u"MPEG_carriage_flags", true, 0, 0, 3) &&
           element->getHexaTextChild(metadata_locator, u"metadata_locator", false, 0, 255) &&
           element->getIntAttribute(program_number, u"program_number", MPEG_carriage_flags <= 2) &&
           element->getIntAttribute(transport_stream_location, u"transport_stream_location", MPEG_carriage_flags == 1) &&
           element->getIntAttribute(transport_stream_id, u"transport_stream_id", MPEG_carriage_flags == 1) &&
           element->getHexaTextChild(private_data, u"private_data", false, 0, 255);
}
