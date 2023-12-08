//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsVideoDepthRangeDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsIntegerUtils.h"

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::VideoDepthRangeDescriptor::VideoDepthRangeDescriptor(DuckContext& duck, const Descriptor& desc) :
    VideoDepthRangeDescriptor()
{
    deserialize(duck, desc);
}

void ts::VideoDepthRangeDescriptor::clearContent()
{
    ranges.clear();
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::VideoDepthRangeDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::VideoDepthRangeDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : ranges) {
        buf.putUInt8(it.range_type);
        buf.pushWriteSequenceWithLeadingLength(8); // range_length
        switch (it.range_type) {
            case 0:
                buf.putBits(it.video_max_disparity_hint, 12);
                buf.putBits(it.video_min_disparity_hint, 12);
                break;
            case 1:
                break;
            default:
                buf.putBytes(it.range_selector);
                break;
        }
        buf.popState(); // update range_length
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::VideoDepthRangeDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Range range;
        range.range_type = buf.getUInt8();
        buf.pushReadSizeFromLength(8); // range_length
        switch (range.range_type) {
            case 0:
                buf.getBits(range.video_max_disparity_hint, 12);
                buf.getBits(range.video_min_disparity_hint, 12);
                break;
            case 1:
                break;
            default:
                buf.getBytes(range.range_selector);
                break;
        }
        buf.popState(); // from range_length
        ranges.push_back(range);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::VideoDepthRangeDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(2)) {
        const uint8_t type = buf.getUInt8();
        disp << margin << UString::Format(u"- Range type: 0x%X (%<d)", {type}) << std::endl;

        buf.pushReadSizeFromLength(8); // range_length
        if (type == 0 && buf.canReadBytes(3)) {
            disp << margin << UString::Format(u"  Video max disparity hint: %d", {buf.getBits<int16_t>(12)});
            disp << UString::Format(u", min: %d", {buf.getBits<int16_t>(12)}) << std::endl;
        }
        else if (type > 1) {
            disp.displayPrivateData(u"Range selector bytes", buf, NPOS, margin + u"  ");
        }
        disp.displayPrivateData(u"Extraneous range selector bytes", buf, NPOS, margin + u"  ");
        buf.popState(); // from range_length
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::VideoDepthRangeDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : ranges) {
        xml::Element* e = root->addElement(u"range");
        e->setIntAttribute(u"range_type", it.range_type, true);
        if (it.range_type == 0) {
            e->setIntAttribute(u"video_max_disparity_hint", it.video_max_disparity_hint);
            e->setIntAttribute(u"video_min_disparity_hint", it.video_min_disparity_hint);
        }
        else if (it.range_type > 1) {
            e->addHexaTextChild(u"range_selector", it.range_selector, true);
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
        ok = xranges[i]->getIntAttribute(range.range_type, u"range_type", true) &&
             xranges[i]->getIntAttribute(range.video_max_disparity_hint, u"video_max_disparity_hint", range.range_type == 0) &&
             xranges[i]->getIntAttribute(range.video_min_disparity_hint, u"video_min_disparity_hint", range.range_type == 0) &&
             xranges[i]->getHexaTextChild(range.range_selector, u"range_selector", false, 0, range.range_type < 2 ? 0 : MAX_DESCRIPTOR_SIZE);
        ranges.push_back(range);
    }
    return ok;
}
