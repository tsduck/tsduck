//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAVCVideoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"AVC_video_descriptor"
#define MY_CLASS ts::AVCVideoDescriptor
#define MY_DID ts::DID_AVC_VIDEO
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AVCVideoDescriptor::AVCVideoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::AVCVideoDescriptor::AVCVideoDescriptor(DuckContext& duck, const Descriptor& desc) :
    AVCVideoDescriptor()
{
    deserialize(duck, desc);
}

void ts::AVCVideoDescriptor::clearContent()
{
    profile_idc = 0;
    constraint_set0 = false;
    constraint_set1 = false;
    constraint_set2 = false;
    constraint_set3 = false;
    constraint_set4 = false;
    constraint_set5 = false;
    AVC_compatible_flags = 0;
    level_idc = 0;
    AVC_still_present = false;
    AVC_24_hour_picture = false;
    frame_packing_SEI_not_present = false;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AVCVideoDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(profile_idc);
    buf.putBit(constraint_set0);
    buf.putBit(constraint_set1);
    buf.putBit(constraint_set2);
    buf.putBit(constraint_set3);
    buf.putBit(constraint_set4);
    buf.putBit(constraint_set5);
    buf.putBits(AVC_compatible_flags, 2);
    buf.putUInt8(level_idc);
    buf.putBit(AVC_still_present);
    buf.putBit(AVC_24_hour_picture);
    buf.putBit(frame_packing_SEI_not_present);
    buf.putBits(0xFF, 5);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AVCVideoDescriptor::deserializePayload(PSIBuffer& buf)
{
    profile_idc = buf.getUInt8();
    constraint_set0 = buf.getBool();
    constraint_set1 = buf.getBool();
    constraint_set2 = buf.getBool();
    constraint_set3 = buf.getBool();
    constraint_set4 = buf.getBool();
    constraint_set5 = buf.getBool();
    buf.getBits(AVC_compatible_flags, 2);
    level_idc = buf.getUInt8();
    AVC_still_present = buf.getBool();
    AVC_24_hour_picture = buf.getBool();
    frame_packing_SEI_not_present = buf.getBool();
    buf.skipBits(5);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AVCVideoDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        disp << margin << "Profile IDC: " << int(buf.getUInt8());
        buf.pushState();
        buf.skipBits(8);
        disp << ", level IDC: " << int(buf.getUInt8()) << std::endl;
        buf.popState();
        disp << margin << "Constraint set0: " << UString::TrueFalse(buf.getBool());
        disp << ", set1: " << UString::TrueFalse(buf.getBool());
        disp << ", set2: " << UString::TrueFalse(buf.getBool());
        disp << ", set3: " << UString::TrueFalse(buf.getBool());
        disp << ", set4: " << UString::TrueFalse(buf.getBool());
        disp << ", set5: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "AVC compatible flags: " << UString::Hexa(buf.getBits<uint8_t>(2)) << std::endl;
        buf.skipBits(8);
        disp << margin << "Still pictures: " << UString::TrueFalse(buf.getBool());
        disp << ", 24-hour pictures: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << "Frame packing SEI not present: " << UString::TrueFalse(buf.getBool()) << std::endl;
        buf.skipBits(5);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AVCVideoDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"profile_idc", profile_idc, true);
    root->setBoolAttribute(u"constraint_set0", constraint_set0);
    root->setBoolAttribute(u"constraint_set1", constraint_set1);
    root->setBoolAttribute(u"constraint_set2", constraint_set2);
    root->setBoolAttribute(u"constraint_set3", constraint_set3);
    root->setBoolAttribute(u"constraint_set4", constraint_set4);
    root->setBoolAttribute(u"constraint_set5", constraint_set5);
    root->setIntAttribute(u"AVC_compatible_flags", AVC_compatible_flags, true);
    root->setIntAttribute(u"level_idc", level_idc, true);
    root->setBoolAttribute(u"AVC_still_present", AVC_still_present);
    root->setBoolAttribute(u"AVC_24_hour_picture", AVC_24_hour_picture);
    root->setBoolAttribute(u"frame_packing_SEI_not_present", frame_packing_SEI_not_present);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AVCVideoDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return  element->getIntAttribute(profile_idc, u"profile_idc", true, 0, 0x00, 0xFF) &&
            element->getBoolAttribute(constraint_set0, u"constraint_set0", true) &&
            element->getBoolAttribute(constraint_set1, u"constraint_set1", true) &&
            element->getBoolAttribute(constraint_set2, u"constraint_set2", true) &&
            element->getBoolAttribute(constraint_set3, u"constraint_set3", false, false) &&
            element->getBoolAttribute(constraint_set4, u"constraint_set4", false, false) &&
            element->getBoolAttribute(constraint_set5, u"constraint_set5", false, false) &&
            element->getIntAttribute(AVC_compatible_flags, u"AVC_compatible_flags", true, 0, 0x00, 0x03) &&
            element->getIntAttribute(level_idc, u"level_idc", true, 0, 0x00, 0xFF) &&
            element->getBoolAttribute(AVC_still_present, u"AVC_still_present", true) &&
            element->getBoolAttribute(AVC_24_hour_picture, u"AVC_24_hour_picture", true) &&
            element->getBoolAttribute(frame_packing_SEI_not_present, u"frame_packing_SEI_not_present", false, false);
}
