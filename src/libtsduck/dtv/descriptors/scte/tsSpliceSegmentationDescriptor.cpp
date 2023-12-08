//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSpliceSegmentationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"splice_segmentation_descriptor"
#define MY_CLASS ts::SpliceSegmentationDescriptor
#define MY_DID ts::DID_SPLICE_SEGMENT
#define MY_TID ts::TID_SCTE35_SIT
#define MY_STD ts::Standards::SCTE

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SpliceSegmentationDescriptor::SpliceSegmentationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::SpliceSegmentationDescriptor::SpliceSegmentationDescriptor(DuckContext& duck, const Descriptor& desc) :
    SpliceSegmentationDescriptor()
{
    deserialize(duck, desc);
}

void ts::SpliceSegmentationDescriptor::clearContent()
{
    identifier = SPLICE_ID_CUEI;
    segmentation_event_id = 0;
    segmentation_event_cancel = false;
    program_segmentation = true;
    web_delivery_allowed = true;
    no_regional_blackout = true;
    archive_allowed = true;
    device_restrictions = 3;
    pts_offsets.clear();
    segmentation_duration.reset();
    segmentation_upid_type = 0;
    segmentation_upid.clear();
    segmentation_type_id = 0;
    segment_num = 0;
    segments_expected = 0;
    sub_segment_num = 0;
    sub_segments_expected = 0;
}


//----------------------------------------------------------------------------
// Rebuild the delivery_not_restricted flag.
//----------------------------------------------------------------------------

bool ts::SpliceSegmentationDescriptor::deliveryNotRestricted() const
{
    return web_delivery_allowed && no_regional_blackout && archive_allowed && device_restrictions == 3;
}

//----------------------------------------------------------------------------
// Check if the signal is an out.
//----------------------------------------------------------------------------

bool ts::SpliceSegmentationDescriptor::isOut() const
{
    switch (segmentation_type_id) {
        case 0x10: // Program Start
        case 0x14: // Program Resumption
        case 0x17: // Program Overlap Start
        case 0x19: // Program Start In Progress
        case 0x20: // Chapter Start
        case 0x22: // Break Start
        case 0x30: // Provider Advertisement Start
        case 0x32: // Distributor Advertisement Start
        case 0x34: // Provider Placement Opportunity Start
        case 0x36: // Distributor Placement Opportunity Start
        case 0x40: // Unscheduled Event Start
        case 0x50: // Network Start
            return true;
        default:
            return false;
    }
}


//----------------------------------------------------------------------------
// Check if the signal is an in.
//----------------------------------------------------------------------------

bool ts::SpliceSegmentationDescriptor::isIn() const
{
    switch (segmentation_type_id) {
        case 0x11: // Program End
        case 0x12: // Program Early Termination
        case 0x13: // Program Breakaway
        case 0x15: // Program Runover Planned
        case 0x16: // Program Runover Unplanned
        case 0x18: // Program Blackout Override
        case 0x21: // Chapter End
        case 0x23: // Break End
        case 0x31: // Provider Advertisement End
        case 0x33: // Distributor Advertisement End
        case 0x35: // Provider Placement Opportunity End
        case 0x37: // Distributor Placement Opportunity End
        case 0x41: // Unscheduled Event End
        case 0x51: // Network End
            return true;
        default:
            return false;
    }
}



//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SpliceSegmentationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt32(identifier);
    buf.putUInt32(segmentation_event_id);
    buf.putBit(segmentation_event_cancel);
    buf.putBits(0xFF, 7);
    if (!segmentation_event_cancel) {
        buf.putBit(program_segmentation);
        buf.putBit(segmentation_duration.has_value());
        buf.putBit(deliveryNotRestricted());
        buf.putBit(web_delivery_allowed);
        buf.putBit(no_regional_blackout);
        buf.putBit(archive_allowed);
        buf.putBits(device_restrictions, 2);
        if (!program_segmentation) {
            buf.putUInt8(uint8_t(pts_offsets.size()));
            for (const auto& it : pts_offsets) {
                buf.putUInt8(it.first);     // component_tag
                buf.putBits(0xFF, 7);
                buf.putBits(it.second, 33); // pts_offset
            }
        }
        if (segmentation_duration.has_value()) {
            buf.putUInt40(segmentation_duration.value());
        }
        buf.putUInt8(segmentation_upid_type);
        buf.putUInt8(uint8_t(segmentation_upid.size()));
        buf.putBytes(segmentation_upid);
        buf.putUInt8(segmentation_type_id);
        buf.putUInt8(segment_num);
        buf.putUInt8(segments_expected);
        if (segmentation_type_id == 0x34 || segmentation_type_id == 0x36 || segmentation_type_id == 0x38 || segmentation_type_id == 0x3A) {
            buf.putUInt8(sub_segment_num);
            buf.putUInt8(sub_segments_expected);
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SpliceSegmentationDescriptor::deserializePayload(PSIBuffer& buf)
{
    identifier = buf.getUInt32();
    segmentation_event_id = buf.getUInt32();
    segmentation_event_cancel = buf.getBool();
    buf.skipBits(7);
    if (!segmentation_event_cancel) {
        program_segmentation = buf.getBool();
        const bool has_duration = buf.getBool();
        const bool not_restricted = buf.getBool();
        if (not_restricted) {
            buf.skipBits(5);
            web_delivery_allowed = true;
            no_regional_blackout = true;
            archive_allowed = true;
            device_restrictions = 3;
        }
        else {
            web_delivery_allowed = buf.getBool();
            no_regional_blackout = buf.getBool();
            archive_allowed = buf.getBool();
            buf.getBits(device_restrictions, 2);
        }
        if (!program_segmentation) {
            const size_t count = buf.getUInt8();
            for (size_t i = 0; i < count && buf.canRead(); ++i) {
                const uint8_t component_tag = buf.getUInt8();
                buf.skipBits(7);
                buf.getBits(pts_offsets[component_tag], 33);
            }
        }
        if (has_duration) {
            segmentation_duration = buf.getUInt40();
        }
        segmentation_upid_type = buf.getUInt8();
        const size_t upid_size = buf.getUInt8();
        buf.getBytes(segmentation_upid, upid_size);
        segmentation_type_id = buf.getUInt8();
        segment_num = buf.getUInt8();
        segments_expected = buf.getUInt8();
        if (segmentation_type_id == 0x34 || segmentation_type_id == 0x36 || segmentation_type_id == 0x38 || segmentation_type_id == 0x3A) {
            sub_segment_num = buf.getUInt8();
            sub_segments_expected = buf.getUInt8();
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SpliceSegmentationDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    bool cancel = false;
    bool program_segmentation = false;
    bool has_duration = false;
    bool not_restricted = false;
    uint8_t type_id = 0;

    if (!buf.canReadBytes(9)) {
        buf.setUserError();
    }
    else {
        // Sometimes, the identifier is made of ASCII characters. Try to display them.
        disp.displayIntAndASCII(u"Identifier: 0x%08X", buf, 4, margin);
        disp << margin << UString::Format(u"Segmentation event id: 0x%X", {buf.getUInt32()});
        cancel = buf.getBool();
        buf.skipBits(7);
        disp << UString::Format(u", cancel: %d", {cancel}) << std::endl;
    }

    if (buf.canReadBytes(1) && !cancel) {
        program_segmentation = buf.getBool();
        has_duration = buf.getBool();
        not_restricted = buf.getBool();
        disp << margin << UString::Format(u"Program segmentation: %d, has duration: %d, not restricted: %d", {program_segmentation, has_duration, not_restricted}) << std::endl;
        if (not_restricted) {
            buf.skipBits(5);
        }
        else {
            disp << margin << UString::Format(u"Web delivery allowed: %d", {buf.getBit()});
            disp << UString::Format(u", no regional blackout: %d", {buf.getBit()}) << std::endl;
            disp << margin << UString::Format(u"Archive allowed: %d", {buf.getBit()});
            disp << UString::Format(u", device restrictions: %d", {buf.getBits<uint8_t>(2)}) << std::endl;
        }
    }

    if (!buf.error() && !cancel && !program_segmentation) {
        if (!buf.canReadBytes(1)) {
            buf.setUserError();
        }
        else {
            size_t count = buf.getUInt8();
            disp << margin << UString::Format(u"Component count: %d", {count}) << std::endl;
            while (buf.canReadBytes(6) && count > 0) {
                count--;
                disp << margin << UString::Format(u"Component tag: %d", {buf.getUInt8()});
                buf.skipBits(7);
                disp << UString::Format(u", PTS offset: %d", {buf.getBits<uint64_t>(33)}) << std::endl;
            }
            if (count != 0) {
                buf.setUserError();
            }
        }
    }

    if (!buf.error() && !cancel && has_duration) {
        if (!buf.canReadBytes(5)) {
            buf.setUserError();
        }
        else {
            disp << margin << UString::Format(u"Segment duration: %d", {buf.getUInt40()}) << std::endl;
        }
    }

    if (!buf.error() && !cancel) {
        if (!buf.canReadBytes(2)) {
            buf.setUserError();
        }
        else {
            disp << margin << UString::Format(u"Segmentation upid type: %s", {DataName(MY_XML_NAME, u"UpIdType", buf.getUInt8(), NamesFlags::HEXA_FIRST)}) << std::endl;
            const size_t upid_size = buf.getUInt8();
            disp.displayPrivateData(u"Upid data", buf, upid_size, margin);
        }
    }

    if (!buf.error() && !cancel) {
        if (!buf.canReadBytes(3)) {
            buf.setUserError();
        }
        else {
            type_id = buf.getUInt8();
            disp << margin << UString::Format(u"Segmentation type id: %s", {DataName(MY_XML_NAME, u"TypeId", type_id, NamesFlags::HEXA_FIRST)}) << std::endl;
            disp << margin << UString::Format(u"Segment number: %d", {buf.getUInt8()});
            disp << UString::Format(u", expected segments: %d", {buf.getUInt8()}) << std::endl;
        }
    }

    if (!buf.error() && !cancel && (type_id == 0x34 || type_id == 0x36 || type_id == 0x38 || type_id == 0x3A)) {
        if (!buf.canReadBytes(2)) {
            buf.setUserError();
        }
        else {
            disp << margin << UString::Format(u"Sub-segment number: %d", {buf.getUInt8()});
            disp << UString::Format(u", expected sub-segments: %d", {buf.getUInt8()}) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SpliceSegmentationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"identifier", identifier, true);
    root->setIntAttribute(u"segmentation_event_id", segmentation_event_id, true);
    root->setBoolAttribute(u"segmentation_event_cancel", segmentation_event_cancel);
    if (!segmentation_event_cancel) {
        if (!deliveryNotRestricted()) {
            root->setBoolAttribute(u"web_delivery_allowed", web_delivery_allowed);
            root->setBoolAttribute(u"no_regional_blackout", no_regional_blackout);
            root->setBoolAttribute(u"archive_allowed", archive_allowed);
            root->setIntAttribute(u"device_restrictions", device_restrictions);
        }
        root->setOptionalIntAttribute(u"segmentation_duration", segmentation_duration);
        root->setIntAttribute(u"segmentation_type_id", segmentation_type_id, true);
        root->setIntAttribute(u"segment_num", segment_num);
        root->setIntAttribute(u"segments_expected", segments_expected);
        if (segmentation_type_id == 0x34 || segmentation_type_id == 0x36) {
            root->setIntAttribute(u"sub_segment_num", sub_segment_num);
            root->setIntAttribute(u"sub_segments_expected", sub_segments_expected);
        }
        xml::Element* upid = root->addElement(u"segmentation_upid");
        upid->setIntAttribute(u"type", segmentation_upid_type, true);
        if (!segmentation_upid.empty()) {
            upid->addHexaText(segmentation_upid);
        }
        if (!program_segmentation) {
            for (const auto& it : pts_offsets) {
                xml::Element* comp = root->addElement(u"component");
                comp->setIntAttribute(u"component_tag", it.first);
                comp->setIntAttribute(u"pts_offset", it.second);
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SpliceSegmentationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok =
        element->getIntAttribute(identifier, u"identifier", false, SPLICE_ID_CUEI) &&
        element->getIntAttribute(segmentation_event_id, u"segmentation_event_id", true) &&
        element->getBoolAttribute(segmentation_event_cancel, u"segmentation_event_cancel", false, false);

    if (ok && !segmentation_event_cancel) {
        xml::ElementVector upid;
        xml::ElementVector comp;
        ok = element->getBoolAttribute(web_delivery_allowed, u"web_delivery_allowed", false, true) &&
             element->getBoolAttribute(no_regional_blackout, u"no_regional_blackout", false, true) &&
             element->getBoolAttribute(archive_allowed, u"archive_allowed", false, true) &&
             element->getIntAttribute(device_restrictions, u"device_restrictions", false, 3, 0, 3) &&
             element->getOptionalIntAttribute(segmentation_duration, u"segmentation_duration", 0, 0x000000FFFFFFFFFF) &&
             element->getIntAttribute(segmentation_type_id, u"segmentation_type_id", true) &&
             element->getIntAttribute(segment_num, u"segment_num", true) &&
             element->getIntAttribute(segments_expected, u"segments_expected", true) &&
             element->getChildren(upid, u"segmentation_upid", 1, 1) &&
             upid[0]->getIntAttribute(segmentation_upid_type, u"type", true) &&
             upid[0]->getHexaText(segmentation_upid, 0, 255) &&
             element->getChildren(comp, u"component", 0, 255);

        if (ok && (segmentation_type_id == 0x34 || segmentation_type_id == 0x36)) {
            ok = element->getIntAttribute(sub_segment_num, u"sub_segment_num", true) &&
                 element->getIntAttribute(sub_segments_expected, u"sub_segments_expected", true);
        }

        for (size_t i = 0; ok && i < comp.size(); ++i) {
            uint8_t tag = 0;
            uint64_t pts = 0;
            ok = comp[i]->getIntAttribute(tag, u"component_tag", true) &&
                 comp[i]->getIntAttribute(pts, u"pts_offset", true, 0, 0, PTS_DTS_MASK);
            pts_offsets[tag] = pts;
        }
        program_segmentation = pts_offsets.empty();
    }
    return ok;
}
