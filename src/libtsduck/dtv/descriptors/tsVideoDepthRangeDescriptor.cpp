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

#include "tsVideoDepthRangeDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsIntegerUtils.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"video_depth_range_descriptor"
#define MY_CLASS ts::VideoDepthRangeDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_VIDEO_DEPTH_RANGE
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::VideoDepthRangeDescriptor::VideoDepthRangeDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    ranges()
{
}

ts::VideoDepthRangeDescriptor::VideoDepthRangeDescriptor(DuckContext& duck, const Descriptor& desc) :
    VideoDepthRangeDescriptor()
{
    deserialize(duck, desc);
}

ts::VideoDepthRangeDescriptor::Range::Range() :
    range_type(0),
    video_max_disparity_hint(0),
    video_min_disparity_hint(0),
    range_selector()
{
}

void ts::VideoDepthRangeDescriptor::clearContent()
{
    ranges.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::VideoDepthRangeDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(MY_EDID);
    for (auto it = ranges.begin(); it != ranges.end(); ++it) {
        bbp->appendUInt8(it->range_type);
        switch (it->range_type) {
            case 0:
                bbp->appendUInt8(3);  // size
                bbp->appendUInt24(uint32_t(uint32_t(it->video_max_disparity_hint & 0x0FFF) << 12) | (it->video_min_disparity_hint & 0x0FFF));
                break;
            case 1:
                bbp->appendUInt8(0);  // size
                break;
            default:
                bbp->appendUInt8(uint8_t(it->range_selector.size()));
                bbp->append(it->range_selector);
                break;
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::VideoDepthRangeDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1 && data[0] == MY_EDID;
    data++; size--;
    ranges.clear();

    while (_is_valid && size >= 2) {
        Range range;
        range.range_type = data[0];
        size_t len = data[1];
        data += 2; size -= 2;

        switch (range.range_type) {
            case 0:
                _is_valid = len == 3;
                if (_is_valid) {
                    const int32_t hint = GetInt24(data);
                    data += 3; size -= 3;
                    range.video_max_disparity_hint = SignExtend(int16_t(hint >> 12), 12);
                    range.video_min_disparity_hint = SignExtend(int16_t(hint), 12);
                }
                break;
            case 1:
                _is_valid = len == 0;
                break;
            default:
                _is_valid = size >= len;
                if (_is_valid) {
                    range.range_selector.copy(data, len);
                    data += len; size -= len;
                }
                break;
        }

        if (_is_valid) {
            ranges.push_back(range);
        }
    }

    // Make sure there is no truncated trailing data.
    _is_valid = _is_valid && size == 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::VideoDepthRangeDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    // Important: With extension descriptors, the DisplayDescriptor() function is called
    // with extension payload. Meaning that data points after descriptor_tag_extension.
    // See ts::TablesDisplay::displayDescriptorData()

    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');
    bool ok = true;

    while (ok && size >= 2) {
        const uint8_t type = data[0];
        size_t len = data[1];
        data += 2; size -= 2;
        strm << margin << UString::Format(u"- Range type: 0x%X (%d)", {type, type}) << std::endl;

        switch (type) {
            case 0:
                ok = len == 3;
                if (ok) {
                    const int32_t hint = GetInt24(data);
                    const int16_t max = SignExtend(int16_t(hint >> 12), 12);
                    const int16_t min = SignExtend(int16_t(hint), 12);
                    data += 3; size -= 3;
                    strm << margin << UString::Format(u"  Video max disparity hint: %d, min: %d", {max, min}) << std::endl;
                }
                break;
            case 1:
                ok = len == 0;
                break;
            default:
                ok = size >= len;
                if (ok) {
                    display.displayPrivateData(u"Range selector bytes", data, len, indent + 2);
                    data += len; size -= len;
                }
                break;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::VideoDepthRangeDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto it = ranges.begin(); it != ranges.end(); ++it) {
        xml::Element* e = root->addElement(u"range");
        e->setIntAttribute(u"range_type", it->range_type, true);
        if (it->range_type == 0) {
            e->setIntAttribute(u"video_max_disparity_hint", it->video_max_disparity_hint);
            e->setIntAttribute(u"video_min_disparity_hint", it->video_min_disparity_hint);
        }
        else if (it->range_type > 1) {
            e->addHexaTextChild(u"range_selector", it->range_selector, true);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::VideoDepthRangeDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xranges;
    bool ok = element->getChildren(xranges, u"range");

    for (size_t i = 0; ok && i < xranges.size(); ++i) {
        Range range;
        ok = xranges[i]->getIntAttribute<uint8_t>(range.range_type, u"range_type", true) &&
             xranges[i]->getIntAttribute<int16_t>(range.video_max_disparity_hint, u"video_max_disparity_hint", range.range_type == 0) &&
             xranges[i]->getIntAttribute<int16_t>(range.video_min_disparity_hint, u"video_min_disparity_hint", range.range_type == 0) &&
             xranges[i]->getHexaTextChild(range.range_selector, u"range_selector", false, 0, range.range_type < 2 ? 0 : MAX_DESCRIPTOR_SIZE);
        ranges.push_back(range);
    }
    return ok;
}
