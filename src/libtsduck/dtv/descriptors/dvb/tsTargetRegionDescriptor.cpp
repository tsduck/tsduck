//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTargetRegionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"target_region_descriptor"
#define MY_CLASS ts::TargetRegionDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_TARGET_REGION
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TargetRegionDescriptor::TargetRegionDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::TargetRegionDescriptor::clearContent()
{
    country_code.clear();
    regions.clear();
}

ts::TargetRegionDescriptor::TargetRegionDescriptor(DuckContext& duck, const Descriptor& desc) :
    TargetRegionDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::TargetRegionDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TargetRegionDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putLanguageCode(country_code);
    for (const auto& it : regions) {
        const bool has_cc = it.country_code.size() == 3;
        buf.putBits(0xFF, 5);
        buf.putBit(has_cc);
        buf.putBits(it.region_depth, 2);
        if (has_cc) {
            buf.putLanguageCode(it.country_code);
        }
        if (it.region_depth >= 1) {
            buf.putUInt8(it.primary_region_code);
            if (it.region_depth >= 2) {
                buf.putUInt8(it.secondary_region_code);
                if (it.region_depth >= 3) {
                    buf.putUInt16(it.tertiary_region_code);
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TargetRegionDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getLanguageCode(country_code);
    while (buf.canRead()) {
        Region region;
        buf.skipBits(5);
        const bool has_cc = buf.getBool();
        buf.getBits(region.region_depth, 2);
        if (has_cc) {
            buf.getLanguageCode(region.country_code);
        }
        if (region.region_depth >= 1) {
            region.primary_region_code = buf.getUInt8();
            if (region.region_depth >= 2) {
                region.secondary_region_code = buf.getUInt8();
                if (region.region_depth >= 3) {
                    region.tertiary_region_code = buf.getUInt16();
                }
            }
        }
        regions.push_back(region);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetRegionDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        disp << margin << "Country code: \"" << buf.getLanguageCode() << "\"" << std::endl;
        for (size_t index = 0; buf.canReadBytes(1); ++index) {
            disp << margin << "- Region #" << index << std::endl;
            buf.skipBits(5);
            const bool has_cc = buf.getBool();
            const uint8_t depth = buf.getBits<uint8_t>(2);
            if (has_cc) {
                disp << margin << "  Country code: \"" << buf.getLanguageCode() << "\"" << std::endl;
            }
            if (depth >= 1) {
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
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetRegionDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"country_code", country_code);
    for (const auto& it : regions) {
        xml::Element* e = root->addElement(u"region");
        e->setAttribute(u"country_code", it.country_code, true);
        if (it.region_depth >= 1) {
            e->setIntAttribute(u"primary_region_code", it.primary_region_code, true);
            if (it.region_depth >= 2) {
                e->setIntAttribute(u"secondary_region_code", it.secondary_region_code, true);
                if (it.region_depth >= 3) {
                    e->setIntAttribute(u"tertiary_region_code", it.tertiary_region_code, true);
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TargetRegionDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xregions;
    bool ok =
        element->getAttribute(country_code, u"country_code", true, u"", 3, 3) &&
        element->getChildren(xregions, u"region");

    for (size_t i = 0; ok && i < xregions.size(); ++i) {
        Region region;
        ok = xregions[i]->getAttribute(region.country_code, u"country_code", false, u"", 3, 3) &&
             xregions[i]->getIntAttribute(region.primary_region_code, u"primary_region_code", false) &&
             xregions[i]->getIntAttribute(region.secondary_region_code, u"secondary_region_code", false) &&
             xregions[i]->getIntAttribute(region.tertiary_region_code, u"tertiary_region_code", false);
        if (xregions[i]->hasAttribute(u"tertiary_region_code")) {
            region.region_depth = 3;
        }
        else if (xregions[i]->hasAttribute(u"secondary_region_code")) {
            region.region_depth = 2;
        }
        else if (xregions[i]->hasAttribute(u"primary_region_code")) {
            region.region_depth = 1;
        }
        else {
            region.region_depth = 0;
        }
        regions.push_back(region);
    }
    return ok;
}
