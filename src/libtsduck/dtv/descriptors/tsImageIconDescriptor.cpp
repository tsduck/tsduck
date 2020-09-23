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

#include "tsImageIconDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"image_icon_descriptor"
#define MY_CLASS ts::ImageIconDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_IMAGE_ICON
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ImageIconDescriptor::ImageIconDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    descriptor_number(0),
    last_descriptor_number(0),
    icon_id(0),
    icon_transport_mode(0),
    has_position(false),
    coordinate_system(0),
    icon_horizontal_origin(0),
    icon_vertical_origin(0),
    icon_type(),
    url(),
    icon_data()
{
}

ts::ImageIconDescriptor::ImageIconDescriptor(DuckContext& duck, const Descriptor& desc) :
    ImageIconDescriptor()
{
    deserialize(duck, desc);
}

void ts::ImageIconDescriptor::clearContent()
{
    descriptor_number = 0;
    last_descriptor_number = 0;
    icon_id = 0;
    icon_transport_mode = 0;
    has_position = false;
    coordinate_system = 0;
    icon_horizontal_origin = 0;
    icon_vertical_origin = 0;
    icon_type.clear();
    url.clear();
    icon_data.clear();
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::ImageIconDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ImageIconDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(descriptor_number, 4);
    buf.putBits(last_descriptor_number, 4);
    buf.putBits(0xFF, 5);
    buf.putBits(icon_id, 3);

    if (descriptor_number == 0) {
        buf.putBits(icon_transport_mode, 2);
        buf.putBit(has_position);
        if (has_position) {
            buf.putBits(coordinate_system, 3);
            buf.putBits(0xFF, 2);
            buf.putBits(icon_horizontal_origin, 12);
            buf.putBits(icon_vertical_origin, 12);
        }
        else {
            buf.putBits(0xFF, 5);
        }
        buf.putStringWithByteLength(icon_type);
        if (icon_transport_mode == 0) {
            buf.putUInt8(uint8_t(icon_data.size()));
            buf.putBytes(icon_data);
        }
        else if (icon_transport_mode == 1) {
            buf.putStringWithByteLength(url);
        }
    }
    else {
        buf.putUInt8(uint8_t(icon_data.size()));
        buf.putBytes(icon_data);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ImageIconDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(descriptor_number, 4);
    buf.getBits(last_descriptor_number, 4);
    buf.skipBits(5);
    buf.getBits(icon_id, 3);

    if (descriptor_number == 0) {
        buf.getBits(icon_transport_mode, 2);
        has_position = buf.getBool();
        if (has_position) {
            buf.getBits(coordinate_system, 3);
            buf.skipBits(2);
            buf.getBits(icon_horizontal_origin, 12);
            buf.getBits(icon_vertical_origin, 12);
        }
        else {
            buf.skipBits(5);
        }
        buf.getStringWithByteLength(icon_type);
        if (icon_transport_mode == 0x00 ) {
            const size_t len = buf.getUInt8();
            buf.getBytes(icon_data, len);
        }
        else if (icon_transport_mode == 0x01) {
            buf.getStringWithByteLength(url);
        }
    }
    else {
        const size_t len = buf.getUInt8();
        buf.getBytes(icon_data, len);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ImageIconDescriptor::DisplayDescriptor(TablesDisplay& disp, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    // Important: With extension descriptors, the DisplayDescriptor() function is called
    // with extension payload. Meaning that data points after descriptor_tag_extension.
    // See ts::TablesDisplay::displayDescriptorData()

    const UString margin(indent, ' ');
    bool ok = size >= 3;

    if (ok) {
        const uint8_t desc = (data[0] >> 4) & 0x0F;
        disp << margin << UString::Format(u"Descriptor number: %d, last: %d", {desc, data[0] & 0x0F}) << std::endl
             << margin << UString::Format(u"Icon id: %d", {data[1] & 0x07}) << std::endl;
        data += 2; size -= 2;

        if (desc == 0) {
            const uint8_t transport = (data[0] >> 6) & 0x03;
            const bool pos = (data[0] & 0x20) != 0;
            const uint8_t coord = (data[0] >> 2) & 0x07;
            data++; size--;

            disp << margin << "Transport mode: " << NameFromSection(u"IconTransportMode", transport, names::DECIMAL_FIRST) << std::endl
                 << margin << "Position specified: " << UString::YesNo(pos) << std::endl;

            if (pos) {
                disp << margin << "Coordinate system: " << NameFromSection(u"IconCoordinateSystem", coord, names::DECIMAL_FIRST) << std::endl;
                ok = size >= 3;
                if (ok) {
                    disp << margin << UString::Format(u"Horizontal origin: %d, vertical: %d", {(GetUInt16(data) >> 4) & 0x0FFF, GetUInt16(data + 1) & 0x0FFF}) << std::endl;
                    data += 3; size -= 3;
                }
            }
            if (ok) {
                disp << margin << "Icon type: \"" << disp.duck().decodedWithByteLength(data, size) << "\"" << std::endl;
                if (transport == 0x00 ) {
                    const size_t len = data[0];
                    ok = size > len;
                    if (ok) {
                        disp.displayPrivateData(u"Icon data", data + 1, len, margin);
                        data += len + 1; size -= len + 1;
                    }
                }
                else if (transport == 0x01) {
                    disp << margin << "URL: \"" << disp.duck().decodedWithByteLength(data, size) << "\"" << std::endl;
                }
            }
        }
        else {
            const size_t len = data[0];
            ok = size > len;
            if (ok) {
                disp.displayPrivateData(u"Icon data", data + 1, len, margin);
                data += len + 1; size -= len + 1;
            }
        }
    }

    disp.displayExtraData(data, size, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ImageIconDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"descriptor_number", descriptor_number);
    root->setIntAttribute(u"last_descriptor_number", last_descriptor_number);
    root->setIntAttribute(u"icon_id", icon_id);

    if (descriptor_number == 0) {
        root->setIntAttribute(u"icon_transport_mode", icon_transport_mode);
        if (has_position) {
            root->setIntAttribute(u"coordinate_system", coordinate_system);
            root->setIntAttribute(u"icon_horizontal_origin", icon_horizontal_origin);
            root->setIntAttribute(u"icon_vertical_origin", icon_vertical_origin);
        }
        root->setAttribute(u"icon_type", icon_type);
        if (icon_transport_mode == 0 && !icon_data.empty()) {
            root->addHexaTextChild(u"icon_data", icon_data);
        }
        else if (icon_transport_mode == 1) {
            root->setAttribute(u"url", url);
        }
    }
    else if (!icon_data.empty()) {
        root->addHexaTextChild(u"icon_data", icon_data);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ImageIconDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    has_position =
        element->hasAttribute(u"coordinate_system") ||
        element->hasAttribute(u"icon_horizontal_origin") ||
        element->hasAttribute(u"icon_vertical_origin");

    return  element->getIntAttribute(descriptor_number, u"descriptor_number", true, 0, 0x00, 0x0F) &&
            element->getIntAttribute(last_descriptor_number, u"last_descriptor_number", true, 0, 0x00, 0x0F) &&
            element->getIntAttribute(icon_id, u"icon_id", true, 0, 0x00, 0x07) &&
            element->getIntAttribute(icon_transport_mode, u"icon_transport_mode", descriptor_number == 0, 0, 0x00, 0x03) &&
            element->getIntAttribute(coordinate_system, u"coordinate_system", descriptor_number == 0 && has_position, 0, 0x00, 0x07) &&
            element->getIntAttribute(icon_horizontal_origin, u"icon_horizontal_origin", descriptor_number == 0 && has_position, 0, 0x0000, 0x0FFF) &&
            element->getIntAttribute(icon_vertical_origin, u"icon_vertical_origin", descriptor_number == 0 && has_position, 0, 0x0000, 0x0FFF) &&
            element->getAttribute(icon_type, u"icon_type", descriptor_number == 0) &&
            element->getAttribute(url, u"url", descriptor_number == 0 && icon_transport_mode == 1) &&
            element->getHexaTextChild(icon_data, u"icon_data", false);
}
