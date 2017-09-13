//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
#include "tsStringUtils.h"
#include "tsTablesFactory.h"
#include "tsXMLTables.h"
TSDUCK_SOURCE;
TS_XML_DESCRIPTOR_FACTORY(ts::AVCVideoDescriptor, "AVC_video_descriptor");
TS_ID_DESCRIPTOR_FACTORY(ts::AVCVideoDescriptor, ts::EDID(ts::DID_AVC_VIDEO));
TS_ID_DESCRIPTOR_DISPLAY(ts::AVCVideoDescriptor::DisplayDescriptor, ts::EDID(ts::DID_AVC_VIDEO));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::AVCVideoDescriptor::AVCVideoDescriptor() :
    AbstractDescriptor(DID_AVC_VIDEO, "AVC_video_descriptor"),
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

ts::AVCVideoDescriptor::AVCVideoDescriptor(const Descriptor& desc) :
    AbstractDescriptor(DID_AVC_VIDEO, "AVC_video_descriptor"),
    profile_idc(0),
    constraint_set0(false),
    constraint_set1(false),
    constraint_set2(false),
    AVC_compatible_flags(0),
    level_idc(0),
    AVC_still_present(false),
    AVC_24_hour_picture(false)
{
    deserialize(desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AVCVideoDescriptor::serialize (Descriptor& desc) const
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

void ts::AVCVideoDescriptor::deserialize (const Descriptor& desc)
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
             << margin << "Constraint set0: " << TrueFalse(constraint_set0)
             << ", set1: " << TrueFalse(constraint_set1)
             << ", set2: " << TrueFalse(constraint_set2)
             << ", AVC compatible flags: " << Format("0x%02X", int(AVC_compatible_flags))
             << std::endl
             << margin << "Still pictures: " << TrueFalse(AVC_still_present)
             << ", 24-hour pictures: " << TrueFalse(AVC_24_hour_picture)
             << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::AVCVideoDescriptor::toXML(XML& xml, XML::Element* parent) const
{
    XML::Element* root = _is_valid ? xml.addElement(parent, _xml_name) : 0;
    xml.setIntAttribute(root, "profile_idc", profile_idc, true);
    xml.setBoolAttribute(root, "constraint_set0", constraint_set0);
    xml.setBoolAttribute(root, "constraint_set1", constraint_set1);
    xml.setBoolAttribute(root, "constraint_set2", constraint_set2);
    xml.setIntAttribute(root, "AVC_compatible_flags", AVC_compatible_flags, true);
    xml.setIntAttribute(root, "level_idc", level_idc, true);
    xml.setBoolAttribute(root, "AVC_still_present", level_idc);
    xml.setBoolAttribute(root, "AVC_24_hour_picture", AVC_24_hour_picture);
    return root;
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::AVCVideoDescriptor::fromXML(XML& xml, const XML::Element* element)
{
    _is_valid =
        checkXMLName(xml, element) &&
        xml.getIntAttribute<uint8_t>(profile_idc, element, "profile_idc", true, 0, 0x00, 0xFF) &&
        xml.getBoolAttribute(constraint_set0, element, "constraint_set0", true) &&
        xml.getBoolAttribute(constraint_set1, element, "constraint_set1", true) &&
        xml.getBoolAttribute(constraint_set2, element, "constraint_set2", true) &&
        xml.getIntAttribute<uint8_t>(AVC_compatible_flags, element, "AVC_compatible_flags", true, 0, 0x00, 0x1F) &&
        xml.getIntAttribute<uint8_t>(level_idc, element, "level_idc", true, 0, 0x00, 0xFF) &&
        xml.getBoolAttribute(AVC_still_present, element, "AVC_still_present", true) &&
        xml.getBoolAttribute(AVC_24_hour_picture, element, "AVC_24_hour_picture", true);
}
