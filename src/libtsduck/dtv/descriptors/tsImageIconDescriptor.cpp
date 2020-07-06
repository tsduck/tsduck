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
// Serialization
//----------------------------------------------------------------------------

void ts::ImageIconDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(MY_EDID);
    bbp->appendUInt8(uint8_t(descriptor_number << 4) | (last_descriptor_number & 0x0F));
    bbp->appendUInt8(0xF8 | icon_id);
    if (descriptor_number == 0) {
        if (has_position) {
            bbp->appendUInt8(uint8_t(icon_transport_mode << 6) | 0x23 | uint8_t((coordinate_system & 0x07) << 2));
            bbp->appendUInt24(uint32_t((icon_horizontal_origin & 0x0FFF) << 12) | (icon_vertical_origin & 0x0FFF));
        }
        else {
            bbp->appendUInt8(uint8_t(icon_transport_mode << 6) | 0x1F);
        }
        bbp->append(duck.encodedWithByteLength(icon_type));
        if (icon_transport_mode == 0) {
            bbp->appendUInt8(uint8_t(icon_data.size()));
            bbp->append(icon_data);
        }
        else if (icon_transport_mode == 1) {
            bbp->append(duck.encodedWithByteLength(url));
        }
    }
    else {
        bbp->appendUInt8(uint8_t(icon_data.size()));
        bbp->append(icon_data);
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ImageIconDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 4 && data[0] == MY_EDID;

    icon_type.clear();
    url.clear();
    icon_data.clear();

    if (_is_valid) {
        descriptor_number = (data[1] >> 4) & 0x0F;
        last_descriptor_number = data[1] & 0x0F;
        icon_id = data[2] & 0x07;
        data += 3; size -= 3;

        if (descriptor_number == 0) {
            icon_transport_mode = (data[0] >> 6) & 0x03;
            has_position = (data[0] & 0x20) != 0;
            coordinate_system = (data[0] >> 2) & 0x07;
            data++; size--;

            if (has_position) {
                _is_valid = size >= 3;
                if (_is_valid) {
                    icon_horizontal_origin = (GetUInt16(data) >> 4) & 0x0FFF;
                    icon_vertical_origin = GetUInt16(data + 1) & 0x0FFF;
                    data += 3; size -= 3;
                }
            }
            if (_is_valid) {
                duck.decodeWithByteLength(icon_type, data, size);
                if (icon_transport_mode == 0x00 ) {
                    const size_t len = data[0];
                    _is_valid = size > len;
                    if (_is_valid) {
                        icon_data.copy(data + 1, len);
                        data += len + 1; size -= len + 1;
                    }
                }
                else if (icon_transport_mode == 0x01) {
                    duck.decodeWithByteLength(url, data, size);
                }
            }
        }
        else {
            const size_t len = data[0];
            _is_valid = size > len;
            if (_is_valid) {
                icon_data.copy(data + 1, len);
                data += len + 1; size -= len + 1;
            }
        }
    }

    // Make sure there is no truncated trailing data.
    _is_valid = _is_valid && size == 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ImageIconDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    // Important: With extension descriptors, the DisplayDescriptor() function is called
    // with extension payload. Meaning that data points after descriptor_tag_extension.
    // See ts::TablesDisplay::displayDescriptorData()

    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');
    bool ok = size >= 3;

    if (ok) {
        const uint8_t desc = (data[0] >> 4) & 0x0F;
        strm << margin << UString::Format(u"Descriptor number: %d, last: %d", {desc, data[0] & 0x0F}) << std::endl
             << margin << UString::Format(u"Icon id: %d", {data[1] & 0x07}) << std::endl;
        data += 2; size -= 2;

        if (desc == 0) {
            const uint8_t transport = (data[0] >> 6) & 0x03;
            const bool pos = (data[0] & 0x20) != 0;
            const uint8_t coord = (data[0] >> 2) & 0x07;
            data++; size--;

            strm << margin << "Transport mode: " << NameFromSection(u"IconTransportMode", transport, names::DECIMAL_FIRST) << std::endl
                 << margin << "Position specified: " << UString::YesNo(pos) << std::endl;

            if (pos) {
                strm << margin << "Coordinate system: " << NameFromSection(u"IconCoordinateSystem", coord, names::DECIMAL_FIRST) << std::endl;
                ok = size >= 3;
                if (ok) {
                    strm << margin << UString::Format(u"Horizontal origin: %d, vertical: %d", {(GetUInt16(data) >> 4) & 0x0FFF, GetUInt16(data + 1) & 0x0FFF}) << std::endl;
                    data += 3; size -= 3;
                }
            }
            if (ok) {
                strm << margin << "Icon type: \"" << duck.decodedWithByteLength(data, size) << "\"" << std::endl;
                if (transport == 0x00 ) {
                    const size_t len = data[0];
                    ok = size > len;
                    if (ok) {
                        display.displayPrivateData(u"Icon data", data + 1, len, indent);
                        data += len + 1; size -= len + 1;
                    }
                }
                else if (transport == 0x01) {
                    strm << margin << "URL: \"" << duck.decodedWithByteLength(data, size) << "\"" << std::endl;
                }
            }
        }
        else {
            const size_t len = data[0];
            ok = size > len;
            if (ok) {
                display.displayPrivateData(u"Icon data", data + 1, len, indent);
                data += len + 1; size -= len + 1;
            }
        }
    }

    display.displayExtraData(data, size, indent);
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

    return  element->getIntAttribute<uint8_t>(descriptor_number, u"descriptor_number", true, 0, 0x00, 0x0F) &&
            element->getIntAttribute<uint8_t>(last_descriptor_number, u"last_descriptor_number", true, 0, 0x00, 0x0F) &&
            element->getIntAttribute<uint8_t>(icon_id, u"icon_id", true, 0, 0x00, 0x07) &&
            element->getIntAttribute<uint8_t>(icon_transport_mode, u"icon_transport_mode", descriptor_number == 0, 0, 0x00, 0x03) &&
            element->getIntAttribute<uint8_t>(coordinate_system, u"coordinate_system", descriptor_number == 0 && has_position, 0, 0x00, 0x07) &&
            element->getIntAttribute<uint16_t>(icon_horizontal_origin, u"icon_horizontal_origin", descriptor_number == 0 && has_position, 0, 0x0000, 0x0FFF) &&
            element->getIntAttribute<uint16_t>(icon_vertical_origin, u"icon_vertical_origin", descriptor_number == 0 && has_position, 0, 0x0000, 0x0FFF) &&
            element->getAttribute(icon_type, u"icon_type", descriptor_number == 0) &&
            element->getAttribute(url, u"url", descriptor_number == 0 && icon_transport_mode == 1) &&
            element->getHexaTextChild(icon_data, u"icon_data", false);
}
