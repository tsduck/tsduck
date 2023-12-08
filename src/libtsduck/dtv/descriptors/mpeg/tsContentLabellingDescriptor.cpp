//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsContentLabellingDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"content_labelling_descriptor"
#define MY_CLASS ts::ContentLabellingDescriptor
#define MY_DID ts::DID_CONTENT_LABELLING
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ContentLabellingDescriptor::ContentLabellingDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::ContentLabellingDescriptor::clearContent()
{
    metadata_application_format = 0;
    metadata_application_format_identifier = 0;
    content_time_base_indicator = 0;
    content_reference_id.clear();
    content_time_base_value = 0;
    metadata_time_base_value = 0;
    content_id = 0;
    time_base_association_data.clear();
    private_data.clear();
}

ts::ContentLabellingDescriptor::ContentLabellingDescriptor(DuckContext& duck, const Descriptor& desc) :
    ContentLabellingDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization.
//----------------------------------------------------------------------------

void ts::ContentLabellingDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(metadata_application_format);
    if (metadata_application_format == 0xFFFF) {
        buf.putUInt32(metadata_application_format_identifier);
    }
    buf.putBit(!content_reference_id.empty());
    buf.putBits(content_time_base_indicator, 4);
    buf.putBits(0xFF, 3);
    if (!content_reference_id.empty()) {
        buf.putUInt8(uint8_t(content_reference_id.size()));
        buf.putBytes(content_reference_id);
    }
    if (content_time_base_indicator == 1 || content_time_base_indicator == 2) {
        buf.putBits(0xFF, 7);
        buf.putBits(content_time_base_value, 33);
        buf.putBits(0xFF, 7);
        buf.putBits(metadata_time_base_value, 33);
    }
    if (content_time_base_indicator == 2) {
        buf.putBit(1);
        buf.putBits(content_id, 7);
    }
    if (content_time_base_indicator >= 3 && content_time_base_indicator <= 7) {
        buf.putUInt8(uint8_t(time_base_association_data.size()));
        buf.putBytes(time_base_association_data);
    }
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization.
//----------------------------------------------------------------------------

void ts::ContentLabellingDescriptor::deserializePayload(PSIBuffer& buf)
{
    metadata_application_format = buf.getUInt16();
    if (metadata_application_format == 0xFFFF) {
        metadata_application_format_identifier = buf.getUInt32();
    }
    const bool content_reference_id_record_flag = buf.getBool();
    buf.getBits(content_time_base_indicator, 4);
    buf.skipBits(3);
    if (content_reference_id_record_flag) {
        const size_t length = buf.getUInt8();
        buf.getBytes(content_reference_id, length);
    }
    if (content_time_base_indicator == 1 || content_time_base_indicator == 2) {
        buf.skipBits(7);
        buf.getBits(content_time_base_value, 33);
        buf.skipBits(7);
        buf.getBits(metadata_time_base_value, 33);
    }
    if (content_time_base_indicator == 2) {
        buf.skipBits(1);
        buf.getBits(content_id, 7);
    }
    if (content_time_base_indicator >= 3 && content_time_base_indicator <= 7) {
        const size_t length = buf.getUInt8();
        buf.getBytes(time_base_association_data, length);
    }
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ContentLabellingDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        const uint16_t format = buf.getUInt16();
        disp << margin << "Metadata application format: " << DataName(MY_XML_NAME, u"application_format", format, NamesFlags::HEXA_FIRST) << std::endl;
        if (format == 0xFFFF && buf.canReadBytes(4)) {
            disp << margin << UString::Format(u"Metadata application format identifier: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;
        }
        const bool content_reference_id_record_flag = buf.getBool();
        const uint8_t time_base_indicator = buf.getBits<uint8_t>(4);
        disp << margin << "Content time base indicator: " << DataName(MY_XML_NAME, u"time_base_indicator", time_base_indicator, NamesFlags::HEXA_FIRST) << std::endl;
        buf.skipBits(3);
        if (content_reference_id_record_flag && buf.canReadBytes(1)) {
            const size_t length = buf.getUInt8();
            disp.displayPrivateData(u"Content reference id", buf, length, margin);
        }
        if (time_base_indicator == 1 || time_base_indicator == 2) {
            buf.skipBits(7);
            disp << margin << UString::Format(u"Content time base: 0x%09X (%<'d)", {buf.getBits<uint64_t>(33)}) << std::endl;
            buf.skipBits(7);
            disp << margin << UString::Format(u"Metadata time base: 0x%09X (%<'d)", {buf.getBits<uint64_t>(33)}) << std::endl;
        }
        if (time_base_indicator == 2) {
            buf.skipBits(1);
            disp << margin << UString::Format(u"Content id: 0x%X (%<d)", {buf.getBits<uint8_t>(7)}) << std::endl;
        }
        if (time_base_indicator >= 3 && time_base_indicator <= 7) {
            const size_t length = buf.getUInt8();
            disp.displayPrivateData(u"Time base association data", buf, length, margin);
        }
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization.
//----------------------------------------------------------------------------

void ts::ContentLabellingDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"metadata_application_format", metadata_application_format, true);
    if (metadata_application_format == 0xFFFF) {
        root->setIntAttribute(u"metadata_application_format_identifier", metadata_application_format_identifier, true);
    }
    root->setIntAttribute(u"content_time_base_indicator", content_time_base_indicator, false);
    root->addHexaTextChild(u"content_reference_id", content_reference_id, true);
    if (content_time_base_indicator == 1 || content_time_base_indicator == 2) {
        root->setIntAttribute(u"content_time_base_value", content_time_base_value, false);
        root->setIntAttribute(u"metadata_time_base_value", metadata_time_base_value, false);
    }
    if (content_time_base_indicator == 2) {
        root->setIntAttribute(u"content_id", content_id, true);
    }
    if (content_time_base_indicator >= 3 && content_time_base_indicator <= 7) {
        root->addHexaTextChild(u"time_base_association_data", time_base_association_data, true);
    }
    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization.
//----------------------------------------------------------------------------

bool ts::ContentLabellingDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(metadata_application_format, u"metadata_application_format", true) &&
           element->getIntAttribute(metadata_application_format_identifier, u"metadata_application_format_identifier", metadata_application_format == 0xFFFF) &&
           element->getIntAttribute(content_time_base_indicator, u"content_time_base_indicator", true, 0, 0, 15) &&
           element->getHexaTextChild(content_reference_id, u"content_reference_id", false, 0, 255) &&
           element->getIntAttribute(content_time_base_value, u"content_time_base_value", content_time_base_indicator == 1 || content_time_base_indicator == 2, 0, 0, 0x1FFFFFFFF) &&
           element->getIntAttribute(metadata_time_base_value, u"metadata_time_base_value", content_time_base_indicator == 1 || content_time_base_indicator == 2, 0, 0, 0x1FFFFFFFF) &&
           element->getIntAttribute(content_id, u"content_id", content_time_base_indicator == 2, 0, 0, 0x7F) &&
           element->getHexaTextChild(time_base_association_data, u"time_base_association_data", false, 0, 255) &&
           element->getHexaTextChild(private_data, u"private_data", false, 0, 255);
}
