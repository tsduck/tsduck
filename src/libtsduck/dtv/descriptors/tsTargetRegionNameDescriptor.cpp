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
// Serialization
//----------------------------------------------------------------------------

void ts::TargetRegionNameDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(MY_EDID);
    if (!SerializeLanguageCode(*bbp, country_code) || !SerializeLanguageCode(*bbp, ISO_639_language_code)) {
        return;
    }
    for (auto it = regions.begin(); it != regions.end(); ++it) {
        ByteBlock name(duck.encodedWithByteLength(it->region_name));
        assert(!name.empty());
        if (name[0] > 0x3F) {
            return;
        }
        name[0] |= uint8_t(it->region_depth << 6);
        bbp->append(name);

        bbp->appendUInt8(it->primary_region_code);
        if (it->region_depth >= 2) {
            bbp->appendUInt8(it->secondary_region_code);
            if (it->region_depth >= 3) {
                bbp->appendUInt16(it->tertiary_region_code);
            }
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TargetRegionNameDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 7 && data[0] == MY_EDID;
    regions.clear();

    if (_is_valid) {
        data++; size--;
        _is_valid = deserializeLanguageCode(country_code, data, size) && deserializeLanguageCode(ISO_639_language_code, data, size);
    }
    while (_is_valid && size >= 2) {
        Region region;
        region.region_depth = (data[0] >> 6) & 0x03;
        const size_t len = data[0] & 0x3F;
        data++; size--;

        _is_valid = size > len;
        if (_is_valid) {
            duck.decode(region.region_name, data, len);
            region.primary_region_code = data[len];
            data += len + 1; size -= len + 1;
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

    // Make sure there is no truncated trailing data.
    _is_valid = _is_valid && size == 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetRegionNameDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    // Important: With extension descriptors, the DisplayDescriptor() function is called
    // with extension payload. Meaning that data points after descriptor_tag_extension.
    // See ts::TablesDisplay::displayDescriptorData()

    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');
    bool ok = size >= 6;
    int index = 0;

    if (ok) {
        strm << margin << "Country code: \"" << DeserializeLanguageCode(data) << "\"" << std::endl
             << margin << "Language code: \"" << DeserializeLanguageCode(data + 3) << "\"" << std::endl;
        data += 6; size -= 6;
    }
    while (ok && size >= 1) {
        strm << margin << "- Region #" << index++ << std::endl;

        const int depth = (data[0] >> 6) & 0x03;
        const size_t len = data[0] & 0x3F;
        data++; size--;

        ok = size > len;
        if (ok) {
            strm << margin << "  Region name: \"" << duck.decoded(data, len) << "\"" << std::endl
                 << margin << UString::Format(u"  Primary region code: 0x%X (%d)", {data[len], data[len]}) << std::endl;
            data += len + 1; size -= len + 1;
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
