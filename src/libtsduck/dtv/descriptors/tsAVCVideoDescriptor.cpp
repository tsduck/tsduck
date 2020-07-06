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

#include "tsAVCVideoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"AVC_video_descriptor"
#define MY_CLASS ts::AVCVideoDescriptor
#define MY_DID ts::DID_AVC_VIDEO
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AVCVideoDescriptor::AVCVideoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    profile_idc(0),
    constraint_set0(false),
    constraint_set1(false),
    constraint_set2(false),
    AVC_compatible_flags(0),
    level_idc(0),
    AVC_still_present(false),
    AVC_24_hour_picture(false)
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
    AVC_compatible_flags = 0;
    level_idc = 0;
    AVC_still_present = false;
    AVC_24_hour_picture = false;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AVCVideoDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());

    bbp->appendUInt8(profile_idc);
    bbp->appendUInt8((constraint_set0 ? 0x80 : 0x00) |
                     (constraint_set1 ? 0x40 : 0x00) |
                     (constraint_set2 ? 0x20 : 0x00) |
                     (AVC_compatible_flags & 0x1F));
    bbp->appendUInt8(level_idc);
    bbp->appendUInt8((AVC_still_present ? 0x80 : 0x00) |
                     (AVC_24_hour_picture ? 0x40 : 0x00) |
                     0x3F);

    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AVCVideoDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() == 4;

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        profile_idc = data[0];
        constraint_set0 = (data[1] & 0x80) != 0;
        constraint_set1 = (data[1] & 0x40) != 0;
        constraint_set2 = (data[1] & 0x20) != 0;
        AVC_compatible_flags = data[1] & 0x1F;
        level_idc = data[2];
        AVC_still_present = (data[3] & 0x80) != 0;
        AVC_24_hour_picture = (data[3] & 0x40) != 0;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AVCVideoDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 4) {
        const uint8_t profile_idc = data[0];
        const bool constraint_set0 = (data[1] & 0x80) != 0;
        const bool constraint_set1 = (data[1] & 0x40) != 0;
        const bool constraint_set2 = (data[1] & 0x20) != 0;
        const uint8_t AVC_compatible_flags = data[1] & 0x1F;
        const uint8_t level_idc = data[2];
        const bool AVC_still_present = (data[3] & 0x80) != 0;
        const bool AVC_24_hour_picture = (data[3] & 0x40) != 0;
        data += 4; size -= 4;

        strm << margin << "Profile IDC: " << int(profile_idc)
             << ", level IDC: " << int(level_idc)
             << std::endl
             << margin << "Constraint set0: " << UString::TrueFalse(constraint_set0)
             << ", set1: " << UString::TrueFalse(constraint_set1)
             << ", set2: " << UString::TrueFalse(constraint_set2)
             << ", AVC compatible flags: " << UString::Hexa(AVC_compatible_flags)
             << std::endl
             << margin << "Still pictures: " << UString::TrueFalse(AVC_still_present)
             << ", 24-hour pictures: " << UString::TrueFalse(AVC_24_hour_picture)
             << std::endl;
    }

    display.displayExtraData(data, size, indent);
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
    root->setIntAttribute(u"AVC_compatible_flags", AVC_compatible_flags, true);
    root->setIntAttribute(u"level_idc", level_idc, true);
    root->setBoolAttribute(u"AVC_still_present", AVC_still_present);
    root->setBoolAttribute(u"AVC_24_hour_picture", AVC_24_hour_picture);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AVCVideoDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return  element->getIntAttribute<uint8_t>(profile_idc, u"profile_idc", true, 0, 0x00, 0xFF) &&
            element->getBoolAttribute(constraint_set0, u"constraint_set0", true) &&
            element->getBoolAttribute(constraint_set1, u"constraint_set1", true) &&
            element->getBoolAttribute(constraint_set2, u"constraint_set2", true) &&
            element->getIntAttribute<uint8_t>(AVC_compatible_flags, u"AVC_compatible_flags", true, 0, 0x00, 0x1F) &&
            element->getIntAttribute<uint8_t>(level_idc, u"level_idc", true, 0, 0x00, 0xFF) &&
            element->getBoolAttribute(AVC_still_present, u"AVC_still_present", true) &&
            element->getBoolAttribute(AVC_24_hour_picture, u"AVC_24_hour_picture", true);
}
