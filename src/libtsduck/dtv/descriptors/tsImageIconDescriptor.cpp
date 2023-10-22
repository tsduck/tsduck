//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsImageIconDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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

void ts::ImageIconDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        const uint8_t desc = buf.getBits<uint8_t>(4);
        disp << margin << UString::Format(u"Descriptor number: %d, last: %d", {desc, buf.getBits<uint8_t>(4)}) << std::endl;
        buf.skipBits(5);
        disp << margin << UString::Format(u"Icon id: %d", {buf.getBits<uint8_t>(3)}) << std::endl;

        if (desc == 0) {
            const uint8_t transport = buf.getBits<uint8_t>(2);
            disp << margin << "Transport mode: " << DataName(MY_XML_NAME, u"TransportMode", transport, NamesFlags::DECIMAL_FIRST) << std::endl;
            const bool has_position = buf.getBool();
            disp << margin << "Position specified: " << UString::YesNo(has_position) << std::endl;
            if (has_position) {
                disp << margin << "Coordinate system: " << DataName(MY_XML_NAME, u"CoordinateSystem", buf.getBits<uint8_t>(3), NamesFlags::DECIMAL_FIRST) << std::endl;
                buf.skipBits(2);
                if (buf.canReadBytes(3)) {
                    disp << margin << UString::Format(u"Horizontal origin: %d", {buf.getBits<uint16_t>(12)});
                    disp << UString::Format(u", vertical: %d", {buf.getBits<uint16_t>(12)}) << std::endl;
                }
            }
            else {
                buf.skipBits(5);
            }
            disp << margin << "Icon type: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
            if (transport == 0x00 && buf.canReadBytes(1)) {
                disp.displayPrivateData(u"Icon data", buf, buf.getUInt8(), margin);
            }
            else if (transport == 0x01 && buf.canReadBytes(1)) {
                disp << margin << "URL: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
            }
        }
        else if (buf.canReadBytes(1)) {
            disp.displayPrivateData(u"Icon data", buf, buf.getUInt8(), margin);
        }
    }
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
