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

#include "tsSpliceSegmentationDescriptor.h"
#include "tsDescriptor.h"
#include "tsSCTE35.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    identifier(SPLICE_ID_CUEI),
    segmentation_event_id(0),
    segmentation_event_cancel(false),
    program_segmentation(true),
    web_delivery_allowed(true),
    no_regional_blackout(true),
    archive_allowed(true),
    device_restrictions(3),
    pts_offsets(),
    segmentation_duration(),
    segmentation_upid_type(0),
    segmentation_upid(),
    segmentation_type_id(0),
    segment_num(0),
    segments_expected(0),
    sub_segment_num(0),
    sub_segments_expected(0)
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
    segmentation_duration.clear();
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
// Serialization
//----------------------------------------------------------------------------

void ts::SpliceSegmentationDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt32(identifier);
    bbp->appendUInt32(segmentation_event_id);
    bbp->appendUInt8(segmentation_event_cancel ? 0xFF : 0x7F);
    if (!segmentation_event_cancel) {
        bbp->appendUInt8((program_segmentation ? 0x80 : 0x00) |
                         (segmentation_duration.set() ? 0x40 : 0x00) |
                         (deliveryNotRestricted() ? 0x20 : 0x00) |
                         (web_delivery_allowed ? 0x10 : 0x00) |
                         (no_regional_blackout ? 0x08 : 0x00) |
                         (archive_allowed ? 0x04 : 0x00) |
                         (device_restrictions & 0x03));
        if (!program_segmentation) {
            bbp->appendUInt8(uint8_t(pts_offsets.size()));
            for (auto it = pts_offsets.begin(); it != pts_offsets.end(); ++it) {
                bbp->appendUInt8(it->first);
                bbp->appendUInt8(((it->second >> 32) & 0x01) == 0x00 ? 0xFE : 0xFF);
                bbp->appendUInt32(uint32_t(it->second));
            }
        }
        if (segmentation_duration.set()) {
            bbp->appendUInt40(segmentation_duration.value());
        }
        bbp->appendUInt8(segmentation_upid_type);
        bbp->appendUInt8(uint8_t(segmentation_upid.size()));
        bbp->append(segmentation_upid);
        bbp->appendUInt8(segmentation_type_id);
        bbp->appendUInt8(segment_num);
        bbp->appendUInt8(segments_expected);
        if (segmentation_type_id == 0x34 || segmentation_type_id == 0x36) {
            bbp->appendUInt8(sub_segment_num);
            bbp->appendUInt8(sub_segments_expected);
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SpliceSegmentationDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    clear();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 9;

    if (_is_valid) {
        identifier = GetUInt32(data);
        segmentation_event_id = GetUInt32(data + 4);
        segmentation_event_cancel = (data[8] & 0x80) != 0;
        data += 9; size -= 9;

        if (segmentation_event_cancel) {
            _is_valid = size == 0;
            return; // end of descriptor
        }

        _is_valid = size > 0;
        bool has_duration = false;
        if (_is_valid) {
            program_segmentation = (data[0] & 0x80) != 0x00;
            has_duration = (data[0] & 0x40) != 0x00;
            const bool not_restricted = (data[0] & 0x20) != 0x00;
            if (not_restricted) {
                web_delivery_allowed = true;
                no_regional_blackout = true;
                archive_allowed = true;
                device_restrictions = 3;
            }
            else {
                web_delivery_allowed = (data[0] & 0x10) != 0x00;
                no_regional_blackout = (data[0] & 0x08) != 0x00;
                archive_allowed = (data[0] & 0x04) != 0x00;
                device_restrictions = data[0] & 0x03;
            }
            data += 1; size -= 1;
        }

        if (_is_valid && !program_segmentation) {
            _is_valid = size > 0 && size > size_t(1 + 6 * data[0]);
            if (_is_valid) {
                size_t count = data[0];
                data += 1; size -= 1;
                while (count > 0) {
                    pts_offsets[data[0]] = GetUInt40(data + 1) & PTS_DTS_MASK;
                    data += 6; size -= 6;
                    count--;
                }
            }
        }

        if (_is_valid && has_duration) {
            _is_valid = size >= 5;
            if (_is_valid) {
                segmentation_duration = GetUInt40(data);
                data += 5; size -= 5;
            }
        }

        _is_valid = _is_valid && size >= 2 && size >= size_t(5 + data[1]);
        if (_is_valid) {
            segmentation_upid_type = data[0];
            const size_t upid_size = data[1];
            segmentation_upid.copy(data + 2, upid_size);
            data += 2 + upid_size; size -= 2 + upid_size;

            segmentation_type_id = data[0];
            segment_num = data[1];
            segments_expected = data[2];
            data += 3; size -= 3;
        }

        if (_is_valid && (segmentation_type_id == 0x34 || segmentation_type_id == 0x36)) {
            _is_valid = size >= 2;
            if (_is_valid) {
                sub_segment_num = data[0];
                sub_segments_expected = data[1];
                data += 2; size -= 2;
            }
        }
    }

    _is_valid = size == 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SpliceSegmentationDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    bool ok = size >= 9;
    const uint8_t cancel = ok ? ((data[8] >> 7) & 0x01) : 0;
    bool program_segmentation = false;
    bool has_duration = false;
    uint8_t type_id = 0;

    if (ok) {
        strm << margin << UString::Format(u"Identifier: 0x%X", {GetUInt32(data)});
        duck.displayIfASCII(data, 4, u" (\"", u"\")");
        strm << std::endl
             << margin << UString::Format(u"Segmentation event id: 0x%X, cancel: %d", {GetUInt32(data + 4), cancel})
             << std::endl;
        data += 9; size -= 9;
        ok = !cancel || size > 0;
    }

    if (ok && !cancel) {
        program_segmentation = (data[0] & 0x80) != 0x00;
        has_duration = (data[0] & 0x40) != 0x00;
        const bool not_restricted = (data[0] & 0x20) != 0x00;
        strm << margin << UString::Format(u"Program segmentation: %d, has duration: %d, not restricted: %d", {program_segmentation, has_duration, not_restricted}) << std::endl;
        if (!not_restricted) {
            const bool web_delivery_allowed = (data[0] & 0x10) != 0x00;
            const bool no_regional_blackout = (data[0] & 0x08) != 0x00;
            const bool archive_allowed = (data[0] & 0x04) != 0x00;
            const uint8_t device_restrictions = data[0] & 0x03;
            strm << margin << UString::Format(u"Web delivery allowed: %d, no regional blackout: %d", {web_delivery_allowed, no_regional_blackout}) << std::endl
                 << margin << UString::Format(u"Archive allowed: %d, device restrictions: %d", {archive_allowed, device_restrictions}) << std::endl;
        }
        data += 1; size -= 1;
    }

    if (ok && !cancel && !program_segmentation) {
        ok = size > 0 && size > size_t(1 + 6 * data[0]);
        if (ok) {
            size_t count = data[0];
            data += 1; size -= 1;
            strm << margin << UString::Format(u"Component count: %d", {count}) << std::endl;
            while (count > 0) {
                strm << margin << UString::Format(u"Component tag: %d, PTS offset: %d", {data[0], GetUInt40(data + 1) & PTS_DTS_MASK}) << std::endl;
                data += 6; size -= 6;
                count--;
            }
        }
    }

    if (ok && !cancel && has_duration) {
        ok = size >= 5;
        if (ok) {
            strm << margin << UString::Format(u"Segment duration: %d", {GetUInt40(data)}) << std::endl;
            data += 5; size -= 5;
        }
    }

    if (ok && !cancel) {
        ok = size >= 2 && size >= size_t(5 + data[1]);
        if (ok) {
            const size_t upid_size = data[1];
            strm << margin << UString::Format(u"Segmentation upid type: %s, %d bytes", {NameFromSection(u"SpliceSegmentationUpIdType", data[0], names::HEXA_FIRST), upid_size}) << std::endl;
            if (upid_size > 0) {
                strm << UString::Dump(data + 2, upid_size, UString::BPL, indent + 2, 16);
            }
            data += 2 + upid_size; size -= 2 + upid_size;

            type_id = data[0];
            strm << margin << UString::Format(u"Segmentation type id: %s", {NameFromSection(u"SpliceSegmentationTypeId", type_id, names::HEXA_FIRST)}) << std::endl
                 << margin << UString::Format(u"Segment number: %d, expected segments: %d", {data[1], data[2]}) << std::endl;
            data += 3; size -= 3;
        }
    }

    if (ok && !cancel && (type_id == 0x34 || type_id == 0x36)) {
        ok = size >= 2;
        strm << margin << UString::Format(u"Sub-segment number: %d, expected sub-segments: %d", {data[0], data[1]}) << std::endl;
        data += 2; size -= 2;
    }

    display.displayExtraData(data, size, indent);
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
            for (auto it = pts_offsets.begin(); it != pts_offsets.end(); ++it) {
                xml::Element* comp = root->addElement(u"component");
                comp->setIntAttribute(u"component_tag", it->first);
                comp->setIntAttribute(u"pts_offset", it->second);
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
        element->getIntAttribute<uint32_t>(identifier, u"identifier", false, SPLICE_ID_CUEI) &&
        element->getIntAttribute<uint32_t>(segmentation_event_id, u"segmentation_event_id", true) &&
        element->getBoolAttribute(segmentation_event_cancel, u"segmentation_event_cancel", false, false);

    if (ok && !segmentation_event_cancel) {
        xml::ElementVector upid;
        xml::ElementVector comp;
        ok = element->getBoolAttribute(web_delivery_allowed, u"web_delivery_allowed", false, true) &&
             element->getBoolAttribute(no_regional_blackout, u"no_regional_blackout", false, true) &&
             element->getBoolAttribute(archive_allowed, u"archive_allowed", false, true) &&
             element->getIntAttribute<uint8_t>(device_restrictions, u"device_restrictions", false, 3, 0, 3) &&
             element->getOptionalIntAttribute<uint64_t>(segmentation_duration, u"segmentation_duration", 0, TS_UCONST64(0x000000FFFFFFFFFF)) &&
             element->getIntAttribute<uint8_t>(segmentation_type_id, u"segmentation_type_id", true) &&
             element->getIntAttribute<uint8_t>(segment_num, u"segment_num", true) &&
             element->getIntAttribute<uint8_t>(segments_expected, u"segments_expected", true) &&
             element->getChildren(upid, u"segmentation_upid", 1, 1) &&
             upid[0]->getIntAttribute<uint8_t>(segmentation_upid_type, u"type", true) &&
             upid[0]->getHexaText(segmentation_upid, 0, 255) &&
             element->getChildren(comp, u"component", 0, 255);

        if (ok && (segmentation_type_id == 0x34 || segmentation_type_id == 0x36)) {
            ok = element->getIntAttribute<uint8_t>(sub_segment_num, u"sub_segment_num", true) &&
                 element->getIntAttribute<uint8_t>(sub_segments_expected, u"sub_segments_expected", true);
        }

        for (size_t i = 0; ok && i < comp.size(); ++i) {
            uint8_t tag = 0;
            uint64_t pts = 0;
            ok = comp[i]->getIntAttribute<uint8_t>(tag, u"component_tag", true) &&
                 comp[i]->getIntAttribute<uint64_t>(pts, u"pts_offset", true, 0, 0, PTS_DTS_MASK);
            pts_offsets[tag] = pts;
        }
        program_segmentation = pts_offsets.empty();
    }
    return ok;
}
