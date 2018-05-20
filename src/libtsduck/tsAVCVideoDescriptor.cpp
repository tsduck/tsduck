//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//
//  Representation of an AVC_video_descriptor
//
//----------------------------------------------------------------------------

#include "tsAVCVideoDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"AVC_video_descriptor"
#define MY_DID ts::DID_AVC_VIDEO

TS_XML_DESCRIPTOR_FACTORY(ts::AVCVideoDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::AVCVideoDescriptor, ts::EDID::Standard(MY_DID));
TS_ID_DESCRIPTOR_DISPLAY(ts::AVCVideoDescriptor::DisplayDescriptor, ts::EDID::Standard(MY_DID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::AVCVideoDescriptor::AVCVideoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    profile_idc(0),
    constraint_set0(false),
    constraint_set1(false),
    constraint_set2(false),
    AVC_compatible_flags(0),
    level_idc(0),
    AVC_still_present(false),
    AVC_24_hour_picture(false)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::AVCVideoDescriptor::AVCVideoDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    AVCVideoDescriptor()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AVCVideoDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    uint8_t data[6];
    data[0] = _tag;
    data[1] = 4;
    data[2] = profile_idc;
    data[3] =
        (constraint_set0 ? 0x80 : 0x00) |
        (constraint_set1 ? 0x40 : 0x00) |
        (constraint_set2 ? 0x20 : 0x00) |
        (AVC_compatible_flags & 0x1F);
    data[4] = level_idc;
    data[5] =
        (AVC_still_present ? 0x80 : 0x00) |
        (AVC_24_hour_picture ? 0x40 : 0x00) |
        0x3F;

    Descriptor d(data, sizeof(data));
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AVCVideoDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() == 4;

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
    std::ostream& strm(display.out());
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

void ts::AVCVideoDescriptor::buildXML(xml::Element* root) const
{
    root->setIntAttribute(u"profile_idc", profile_idc, true);
    root->setBoolAttribute(u"constraint_set0", constraint_set0);
    root->setBoolAttribute(u"constraint_set1", constraint_set1);
    root->setBoolAttribute(u"constraint_set2", constraint_set2);
    root->setIntAttribute(u"AVC_compatible_flags", AVC_compatible_flags, true);
    root->setIntAttribute(u"level_idc", level_idc, true);
    root->setBoolAttribute(u"AVC_still_present", level_idc);
    root->setBoolAttribute(u"AVC_24_hour_picture", AVC_24_hour_picture);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::AVCVideoDescriptor::fromXML(const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint8_t>(profile_idc, u"profile_idc", true, 0, 0x00, 0xFF) &&
        element->getBoolAttribute(constraint_set0, u"constraint_set0", true) &&
        element->getBoolAttribute(constraint_set1, u"constraint_set1", true) &&
        element->getBoolAttribute(constraint_set2, u"constraint_set2", true) &&
        element->getIntAttribute<uint8_t>(AVC_compatible_flags, u"AVC_compatible_flags", true, 0, 0x00, 0x1F) &&
        element->getIntAttribute<uint8_t>(level_idc, u"level_idc", true, 0, 0x00, 0xFF) &&
        element->getBoolAttribute(AVC_still_present, u"AVC_still_present", true) &&
        element->getBoolAttribute(AVC_24_hour_picture, u"AVC_24_hour_picture", true);
}
