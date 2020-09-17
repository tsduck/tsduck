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

#include "tsTargetRegionNameDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"target_region_name_descriptor"
#define MY_CLASS ts::TargetRegionNameDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_TARGET_REGION_NAME
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TargetRegionNameDescriptor::TargetRegionNameDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    country_code(),
    ISO_639_language_code(),
    regions()
{
}

void ts::TargetRegionNameDescriptor::clearContent()
{
    country_code.clear();
    ISO_639_language_code.clear();
    regions.clear();
}

ts::TargetRegionNameDescriptor::TargetRegionNameDescriptor(DuckContext& duck, const Descriptor& desc) :
    TargetRegionNameDescriptor()
{
    deserialize(duck, desc);
}

ts::TargetRegionNameDescriptor::Region::Region() :
    region_depth(0),
    region_name(),
    primary_region_code(0),
    secondary_region_code(0),
    tertiary_region_code(0)
{
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::TargetRegionNameDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TargetRegionNameDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putLanguageCode(country_code);
    buf.putLanguageCode(ISO_639_language_code);
    for (auto it = regions.begin(); it != regions.end(); ++it) {
        buf.pushState();
        buf.putStringWithByteLength(it->region_name);
        buf.swapState();
        buf.putBits(it->region_depth, 2);
        buf.popState();
        buf.putUInt8(it->primary_region_code);
        if (it->region_depth >= 2) {
            buf.putUInt8(it->secondary_region_code);
            if (it->region_depth >= 3) {
                buf.putUInt16(it->tertiary_region_code);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TargetRegionNameDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getLanguageCode(country_code);
    buf.getLanguageCode(ISO_639_language_code);
    while (buf.canRead()) {
        Region region;
        region.region_depth = buf.getBits<uint8_t>(2);
        const size_t len = buf.getBits<uint8_t>(6);
        buf.getString(region.region_name, len);
        region.primary_region_code = buf.getUInt8();
        if (region.region_depth >= 2) {
            region.secondary_region_code = buf.getUInt8();
            if (region.region_depth >= 3) {
                region.tertiary_region_code = buf.getUInt16();
            }
        }
        regions.push_back(region);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetRegionNameDescriptor::DisplayDescriptor(TablesDisplay& disp, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    // Important: With extension descriptors, the DisplayDescriptor() function is called
    // with extension payload. Meaning that data points after descriptor_tag_extension.
    // See ts::TablesDisplay::displayDescriptorData()

    const UString margin(indent, ' ');
    bool ok = size >= 6;
    int index = 0;

    if (ok) {
        disp << margin << "Country code: \"" << DeserializeLanguageCode(data) << "\"" << std::endl
             << margin << "Language code: \"" << DeserializeLanguageCode(data + 3) << "\"" << std::endl;
        data += 6; size -= 6;
    }
    while (ok && size >= 1) {
        disp << margin << "- Region #" << index++ << std::endl;

        const int depth = (data[0] >> 6) & 0x03;
        const size_t len = data[0] & 0x3F;
        data++; size--;

        ok = size > len;
        if (ok) {
            disp << margin << "  Region name: \"" << disp.duck().decoded(data, len) << "\"" << std::endl
                 << margin << UString::Format(u"  Primary region code: 0x%X (%d)", {data[len], data[len]}) << std::endl;
            data += len + 1; size -= len + 1;
        }
        if (ok && depth >= 2) {
            ok = size >= 1;
            if (ok) {
                disp << margin << UString::Format(u"  Secondary region code: 0x%X (%d)", {data[0], data[0]}) << std::endl;
                data++; size--;
            }
        }
        if (ok && depth >= 3) {
            ok = size >= 2;
            if (ok) {
                disp << margin << UString::Format(u"  Tertiary region code: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl;
                data += 2; size -= 2;
            }
        }
    }

    disp.displayExtraData(data, size, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetRegionNameDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"country_code", country_code);
    root->setAttribute(u"ISO_639_language_code", ISO_639_language_code);
    for (auto it = regions.begin(); it != regions.end(); ++it) {
        xml::Element* e = root->addElement(u"region");
        e->setAttribute(u"region_name", it->region_name, true);
        e->setIntAttribute(u"primary_region_code", it->primary_region_code, true);
        if (it->region_depth >= 2) {
            e->setIntAttribute(u"secondary_region_code", it->secondary_region_code, true);
            if (it->region_depth >= 3) {
                e->setIntAttribute(u"tertiary_region_code", it->tertiary_region_code, true);
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TargetRegionNameDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xregions;
    bool ok =
        element->getAttribute(country_code, u"country_code", true, u"", 3, 3) &&
        element->getAttribute(ISO_639_language_code, u"ISO_639_language_code", true, u"", 3, 3) &&
        element->getChildren(xregions, u"region");

    for (size_t i = 0; ok && i < xregions.size(); ++i) {
        Region region;
        ok = xregions[i]->getAttribute(region.region_name, u"region_name", true) &&
             xregions[i]->getIntAttribute<uint8_t>(region.primary_region_code, u"primary_region_code", true) &&
             xregions[i]->getIntAttribute<uint8_t>(region.secondary_region_code, u"secondary_region_code", false) &&
             xregions[i]->getIntAttribute<uint16_t>(region.tertiary_region_code, u"tertiary_region_code", false);
        if (xregions[i]->hasAttribute(u"tertiary_region_code")) {
            region.region_depth = 3;
        }
        else if (xregions[i]->hasAttribute(u"secondary_region_code")) {
            region.region_depth = 2;
        }
        else {
            region.region_depth = 1;
        }
        regions.push_back(region);
    }
    return ok;
}
