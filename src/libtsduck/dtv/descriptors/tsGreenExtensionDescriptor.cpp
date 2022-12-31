//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsGreenExtensionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"green_extension_descriptor"
#define MY_CLASS ts::GreenExtensionDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_GREEN_EXT
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::GreenExtensionDescriptor::GreenExtensionDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    constant_backlight_voltage_time_intervals(),
    max_variations()
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
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::GreenExtensionDescriptor::extendedTag() const
{
    return MY_EDID;
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

void ts::GreenExtensionDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        size_t count = buf.getBits<size_t>(2);
        buf.skipBits(6);
        disp << margin << UString::Format(u"Number of backlight voltage time intervals: %d", {count}) << std::endl;
        for (size_t i = 0; i < count && !buf.error(); ++i) {
            disp << margin << UString::Format(u"  Constant backlight voltage time intervals [%d]: 0x%X (%<d)", {i, buf.getUInt16()}) << std::endl;
        }
        buf.getBits(count, 2);
        buf.skipBits(6);
        disp << margin << UString::Format(u"Number of variations: %d", {count}) << std::endl;
        for (size_t i = 0; i < count && buf.canReadBytes(2); ++i) {
            disp << margin << UString::Format(u"  Max variation [%d]: 0x%X (%<d)", {i, buf.getUInt16()}) << std::endl;
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
    xml::ElementVector xvoltage;
    xml::ElementVector xvariation;
    uint16_t id = 0;

    bool ok =
        element->getChildren(xvoltage, u"constant_backlight_voltage_time_interval", 0, MAX_COUNT) &&
        element->getChildren(xvariation, u"max_variation", 0, MAX_COUNT);

    for (auto it = xvoltage.begin(); ok && it != xvoltage.end(); ++it) {
        ok = (*it)->getIntAttribute(id, u"value", true);
        constant_backlight_voltage_time_intervals.push_back(id);
    }
    for (auto it = xvariation.begin(); ok && it != xvariation.end(); ++it) {
        ok = (*it)->getIntAttribute(id, u"value", true);
        max_variations.push_back(id);
    }
    return ok;
}
