//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsGreenExtensionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"green_extension_descriptor"
#define MY_CLASS    ts::GreenExtensionDescriptor
#define MY_EDID     ts::EDID::ExtensionMPEG(ts::XDID_MPEG_GREEN_EXT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::GreenExtensionDescriptor::GreenExtensionDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::GreenExtensionDescriptor::GreenExtensionDescriptor(DuckContext& duck, const Descriptor& desc) :
    GreenExtensionDescriptor()
{
    deserialize(duck, desc);
}

void ts::GreenExtensionDescriptor::clearContent()
{
    constant_backlight_voltage_time_intervals.clear();
    max_variations.clear();
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::GreenExtensionDescriptor::serializePayload(PSIBuffer& buf) const
{
    if (constant_backlight_voltage_time_intervals.size() > MAX_COUNT || max_variations.size() > MAX_COUNT) {
        buf.setUserError();
    }
    else {
        buf.putBits(constant_backlight_voltage_time_intervals.size(), 2);
        buf.putBits(0xFF, 6);
        for (auto it : constant_backlight_voltage_time_intervals) {
            buf.putUInt16(it);
        }
        buf.putBits(max_variations.size(), 2);
        buf.putBits(0xFF, 6);
        for (auto it : max_variations) {
            buf.putUInt16(it);
        }
    }
}

void ts::GreenExtensionDescriptor::deserializePayload(PSIBuffer& buf)
{
    size_t count = buf.getBits<size_t>(2);
    buf.skipBits(6);
    for (size_t i = 0; i < count && !buf.error(); ++i) {
        constant_backlight_voltage_time_intervals.push_back(buf.getUInt16());
    }
    buf.getBits(count, 2);
    buf.skipBits(6);
    for (size_t i = 0; i < count && !buf.error(); ++i) {
        max_variations.push_back(buf.getUInt16());
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::GreenExtensionDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(1)) {
        size_t count = buf.getBits<size_t>(2);
        buf.skipBits(6);
        disp << margin << UString::Format(u"Number of backlight voltage time intervals: %d", count) << std::endl;
        for (size_t i = 0; i < count && !buf.error(); ++i) {
            disp << margin << UString::Format(u"  Constant backlight voltage time intervals [%d]: %n", i, buf.getUInt16()) << std::endl;
        }
        buf.getBits(count, 2);
        buf.skipBits(6);
        disp << margin << UString::Format(u"Number of variations: %d", count) << std::endl;
        for (size_t i = 0; i < count && buf.canReadBytes(2); ++i) {
            disp << margin << UString::Format(u"  Max variation [%d]: %n", i, buf.getUInt16()) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization / deserialization
//----------------------------------------------------------------------------

void ts::GreenExtensionDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto it : constant_backlight_voltage_time_intervals) {
        root->addElement(u"constant_backlight_voltage_time_interval")->setIntAttribute(u"value", it);
    }
    for (auto it : max_variations) {
        root->addElement(u"max_variation")->setIntAttribute(u"value", it);
    }
}

bool ts::GreenExtensionDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = true;
    for (auto& xvoltage : element->children(u"constant_backlight_voltage_time_interval", &ok, 0, MAX_COUNT)) {
        ok = xvoltage.getIntAttribute(constant_backlight_voltage_time_intervals.emplace_back(), u"value", true);
    }
    for (auto& xvariation : element->children(u"max_variation", &ok, 0, MAX_COUNT)) {
        ok = xvariation.getIntAttribute(max_variations.emplace_back(), u"value", true);
    }
    return ok;
}
