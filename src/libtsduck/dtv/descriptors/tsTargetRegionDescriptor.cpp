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

#include "tsTargetRegionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    country_code(),
    regions()
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

ts::TargetRegionDescriptor::Region::Region() :
    country_code(),
    region_depth(0),
    primary_region_code(0),
    secondary_region_code(0),
    tertiary_region_code(0)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TargetRegionDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(MY_EDID);
    if (!SerializeLanguageCode(*bbp, country_code)) {
        return;
    }
    for (auto it = regions.begin(); it != regions.end(); ++it) {
        const bool has_cc = it->country_code.size() == 3;
        bbp->appendUInt8((has_cc ? 0xFC : 0xF8) | (it->region_depth & 0x03));
        if (has_cc) {
            SerializeLanguageCode(*bbp, it->country_code);
        }
        if (it->region_depth >= 1) {
            bbp->appendUInt8(it->primary_region_code);
            if (it->region_depth >= 2) {
                bbp->appendUInt8(it->secondary_region_code);
                if (it->region_depth >= 3) {
                    bbp->appendUInt16(it->tertiary_region_code);
                }
            }
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TargetRegionDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 4 && data[0] == MY_EDID;
    regions.clear();

    if (_is_valid) {
        data++; size--;
        _is_valid = deserializeLanguageCode(country_code, data, size);
    }
    while (_is_valid && size >= 1) {
        Region region;
        region.region_depth = data[0] & 0x03;
        const bool has_cc = (data[0] & 0x04) != 0;
        data++; size--;

        if (has_cc) {
            _is_valid = deserializeLanguageCode(region.country_code, data, size);
        }
        if (_is_valid && region.region_depth >= 1) {
            _is_valid = size >= 1;
            if (_is_valid) {
                region.primary_region_code = data[0];
                data++; size--;
            }
        }
        if (_is_valid && region.region_depth >= 2) {
            _is_valid = size >= 1;
            if (_is_valid) {
                region.secondary_region_code = data[0];
                data++; size--;
            }
        }
        if (_is_valid && region.region_depth >= 3) {
            _is_valid = size >= 2;
            if (_is_valid) {
                region.tertiary_region_code = GetUInt16(data);
                data += 2; size -= 2;
            }
        }
        if (_is_valid) {
            regions.push_back(region);
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetRegionDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    // Important: With extension descriptors, the DisplayDescriptor() function is called
    // with extension payload. Meaning that data points after descriptor_tag_extension.
    // See ts::TablesDisplay::displayDescriptorData()

    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');
    bool ok = size >= 3;
    int index = 0;

    if (ok) {
        strm << margin << "Country code: \"" << DeserializeLanguageCode(data) << "\"" << std::endl;
        data += 3; size -= 3;
    }
    while (ok && size >= 1) {
        strm << margin << "- Region #" << index++ << std::endl;

        const int depth = data[0] & 0x03;
        const bool has_cc = (data[0] & 0x04) != 0;
        data++; size--;

        if (has_cc) {
            ok = size >= 3;
            if (ok) {
                strm << margin << "  Country code: \"" << DeserializeLanguageCode(data) << "\"" << std::endl;
                data += 3; size -= 3;
            }
        }
        if (ok && depth >= 1) {
            ok = size >= 1;
            if (ok) {
                strm << margin << UString::Format(u"  Primary region code: 0x%X (%d)", {data[0], data[0]}) << std::endl;
                data++; size--;
            }
        }
        if (ok && depth >= 2) {
            ok = size >= 1;
            if (ok) {
                strm << margin << UString::Format(u"  Secondary region code: 0x%X (%d)", {data[0], data[0]}) << std::endl;
                data++; size--;
            }
        }
        if (ok && depth >= 3) {
            ok = size >= 2;
            if (ok) {
                strm << margin << UString::Format(u"  Tertiary region code: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl;
                data += 2; size -= 2;
            }
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetRegionDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"country_code", country_code);
    for (auto it = regions.begin(); it != regions.end(); ++it) {
        xml::Element* e = root->addElement(u"region");
        e->setAttribute(u"country_code", it->country_code, true);
        if (it->region_depth >= 1) {
            e->setIntAttribute(u"primary_region_code", it->primary_region_code, true);
            if (it->region_depth >= 2) {
                e->setIntAttribute(u"secondary_region_code", it->secondary_region_code, true);
                if (it->region_depth >= 3) {
                    e->setIntAttribute(u"tertiary_region_code", it->tertiary_region_code, true);
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
             xregions[i]->getIntAttribute<uint8_t>(region.primary_region_code, u"primary_region_code", false) &&
             xregions[i]->getIntAttribute<uint8_t>(region.secondary_region_code, u"secondary_region_code", false) &&
             xregions[i]->getIntAttribute<uint16_t>(region.tertiary_region_code, u"tertiary_region_code", false);
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
