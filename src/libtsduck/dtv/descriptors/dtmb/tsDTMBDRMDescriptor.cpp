//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025-2026, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDTMBDRMDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"DTMB_DRM_descriptor"
#define MY_CLASS    ts::DTMBDRMDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_DTMB_DRM, ts::Standards::DTMB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DTMBDRMDescriptor::DTMBDRMDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::DTMBDRMDescriptor::clearContent()
{
    video_format = 0;
    video_encryption_method = 0;
    audio_format = 0;
    audio_encryption_method = 0;
    DRM_data_bytes.clear();
}

ts::DTMBDRMDescriptor::DTMBDRMDescriptor(DuckContext& duck, const Descriptor& desc) :
    DTMBDRMDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DTMBDRMDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(video_format, 4);
    buf.putBits(video_encryption_method, 4);
    buf.putBits(audio_format, 4);
    buf.putBits(audio_encryption_method, 4);
    buf.putBytes(DRM_data_bytes);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DTMBDRMDescriptor::deserializePayload(PSIBuffer& buf)
{
    video_format = buf.getBits<uint8_t>(4);
    video_encryption_method = buf.getBits<uint8_t>(4);
    audio_format = buf.getBits<uint8_t>(4);
    audio_encryption_method = buf.getBits<uint8_t>(4);
    buf.getBytes(DRM_data_bytes);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DTMBDRMDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(2)) {
        disp << margin << "Video format: " << DataName(MY_XML_NAME, u"video_format", buf.getBits<uint8_t>(4), NamesFlags::NAME_VALUE | NamesFlags::HEXA) << std::endl;
        disp << margin << "Video encryption method: " << DataName(MY_XML_NAME, u"video_encryption_method", buf.getBits<uint8_t>(4), NamesFlags::NAME_VALUE | NamesFlags::HEXA) << std::endl;
        disp << margin << "Audio format: " << UString::Format(u"0x%1X", buf.getBits<uint8_t>(4));
        disp << ", Audio encryption method: " << UString::Format(u"0x%1X", buf.getBits<uint8_t>(4)) << std::endl;
        disp.displayPrivateData(u"DRM data types", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DTMBDRMDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"video_format", video_format, true);
    root->setIntAttribute(u"video_encryption_method", video_encryption_method, true);
    root->setIntAttribute(u"audio_format", audio_format, true);
    root->setIntAttribute(u"audio_encryption_method", audio_encryption_method, true);
    root->addHexaTextChild(u"DRM_data_bytes", DRM_data_bytes, false);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DTMBDRMDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(video_format, u"video_format", true, 0, 0, 0xF) &&
           element->getIntAttribute(video_encryption_method, u"video_encryption_method", true, 0, 0, 0xF) &&
           element->getIntAttribute(audio_format, u"audio_format", true, 0, 0, 0xF) &&
           element->getIntAttribute(audio_encryption_method, u"audio_encryption_method", true, 0, 0, 0xF) &&
           element->getHexaTextChild(DRM_data_bytes, u"DRM_data_bytes", true, 0, 253);
}
