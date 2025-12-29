//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMPEG2AACAudioDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"MPEG2_AAC_audio_descriptor"
#define MY_CLASS    ts::MPEG2AACAudioDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_MPEG_MPEG2_AAC_AUDIO, ts::Standards::MPEG)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MPEG2AACAudioDescriptor::MPEG2AACAudioDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::MPEG2AACAudioDescriptor::MPEG2AACAudioDescriptor(DuckContext& duck, const Descriptor& desc) :
    MPEG2AACAudioDescriptor()
{
    deserialize(duck, desc);
}

void ts::MPEG2AACAudioDescriptor::clearContent()
{
    MPEG2_AAC_profile = 0;
    MPEG2_AAC_channel_configuration = 0;
    MPEG2_AAC_additional_information = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MPEG2AACAudioDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(MPEG2_AAC_profile);
    buf.putUInt8(MPEG2_AAC_channel_configuration);
    buf.putUInt8(MPEG2_AAC_additional_information);
}

void ts::MPEG2AACAudioDescriptor::deserializePayload(PSIBuffer& buf)
{
    MPEG2_AAC_profile = buf.getUInt8();
    MPEG2_AAC_channel_configuration = buf.getUInt8();
    MPEG2_AAC_additional_information = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MPEG2AACAudioDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(3)) {
        disp << margin << UString::Format(u"MPEG-2 AAC profile: %n", buf.getUInt8()) << std::endl;
        disp << margin << UString::Format(u"MPEG-2 AAC channel configuration: %n", buf.getUInt8()) << std::endl;
        disp << margin << UString::Format(u"MPEG-2 AAC additional information: %n", buf.getUInt8()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MPEG2AACAudioDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"MPEG2_AAC_profile", MPEG2_AAC_profile, true);
    root->setIntAttribute(u"MPEG2_AAC_channel_configuration", MPEG2_AAC_channel_configuration, true);
    root->setIntAttribute(u"MPEG2_AAC_additional_information", MPEG2_AAC_additional_information, true);
}

bool ts::MPEG2AACAudioDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(MPEG2_AAC_profile, u"MPEG2_AAC_profile", true) &&
           element->getIntAttribute(MPEG2_AAC_channel_configuration, u"MPEG2_AAC_channel_configuration", true) &&
           element->getIntAttribute(MPEG2_AAC_additional_information, u"MPEG2_AAC_additional_information", true);
}
