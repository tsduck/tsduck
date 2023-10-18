//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTargetRegionNameDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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
    for (const auto& it : regions) {
        buf.pushState();
        buf.putStringWithByteLength(it.region_name);
        buf.swapState();
        buf.putBits(it.region_depth, 2);
        buf.popState();
        buf.putUInt8(it.primary_region_code);
        if (it.region_depth >= 2) {
            buf.putUInt8(it.secondary_region_code);
            if (it.region_depth >= 3) {
                buf.putUInt16(it.tertiary_region_code);
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
        buf.getBits(region.region_depth, 2);
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

void ts::TargetRegionNameDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(6)) {
        disp << margin << "Country code: \"" << buf.getLanguageCode() << "\"" << std::endl;
        disp << margin << "Language code: \"" << buf.getLanguageCode() << "\"" << std::endl;
        for (size_t index = 0; buf.canReadBytes(1); ++index) {
            disp << margin << "- Region #" << index << std::endl;
            const uint8_t depth = buf.getBits<uint8_t>(2);
            const size_t len = buf.getBits<uint8_t>(6);
            disp << margin << "  Region name: \"" << buf.getString(len) << "\"" << std::endl;
            disp << margin << UString::Format(u"  Primary region code: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
            if (depth >= 2) {
                disp << margin << UString::Format(u"  Secondary region code: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
                if (depth >= 3) {
                    disp << margin << UString::Format(u"  Tertiary region code: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetRegionNameDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"country_code", country_code);
    root->setAttribute(u"ISO_639_language_code", ISO_639_language_code);
    for (const auto& it : regions) {
        xml::Element* e = root->addElement(u"region");
        e->setAttribute(u"region_name", it.region_name, true);
        e->setIntAttribute(u"primary_region_code", it.primary_region_code, true);
        if (it.region_depth >= 2) {
            e->setIntAttribute(u"secondary_region_code", it.secondary_region_code, true);
            if (it.region_depth >= 3) {
                e->setIntAttribute(u"tertiary_region_code", it.tertiary_region_code, true);
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
             xregions[i]->getIntAttribute(region.primary_region_code, u"primary_region_code", true) &&
             xregions[i]->getIntAttribute(region.secondary_region_code, u"secondary_region_code", false) &&
             xregions[i]->getIntAttribute(region.tertiary_region_code, u"tertiary_region_code", false);
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
