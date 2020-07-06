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

#include "tsAnnouncementSupportDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"announcement_support_descriptor"
#define MY_CLASS ts::AnnouncementSupportDescriptor
#define MY_DID ts::DID_ANNOUNCE_SUPPORT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AnnouncementSupportDescriptor::AnnouncementSupportDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    announcements()
{
}

void ts::AnnouncementSupportDescriptor::clearContent()
{
    announcements.clear();
}

ts::AnnouncementSupportDescriptor::AnnouncementSupportDescriptor(DuckContext& duck, const Descriptor& desc) :
    AnnouncementSupportDescriptor()
{
    deserialize(duck, desc);
}

ts::AnnouncementSupportDescriptor::Announcement::Announcement(uint8_t type) :
    announcement_type(type),
    reference_type(0),
    original_network_id(0),
    transport_stream_id(0),
    service_id(0),
    component_tag(0)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AnnouncementSupportDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    // Rebuild announcement_support_indicator
    uint16_t indicator = 0;
    for (auto it = announcements.begin(); it != announcements.end(); ++it) {
        indicator |= uint16_t(1 << it->announcement_type);
    }

    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt16(indicator);
    for (auto it = announcements.begin(); it != announcements.end(); ++it) {
        bbp->appendUInt8(uint8_t(it->announcement_type << 4) | 0x08 | (it->reference_type & 0x07));
        if (it->reference_type >= 1 && it->reference_type <= 3) {
            bbp->appendUInt16(it->original_network_id);
            bbp->appendUInt16(it->transport_stream_id);
            bbp->appendUInt16(it->service_id);
            bbp->appendUInt8(it->component_tag);
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AnnouncementSupportDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 2;
    announcements.clear();

    if (!_is_valid) {
        return;
    }

    // Get announcement_support_indicator.
    // We will check later that all annoucement types are present.
    uint16_t indicator = GetUInt16(data);
    data += 2; size -= 2;

    while (_is_valid && size >= 1) {
        Announcement ann;
        ann.announcement_type = (data[0] >> 4) & 0x0F;
        ann.reference_type = data[0] & 0x07;
        data++; size--;

        // Clear types one by one in announcement_support_indicator.
        indicator &= ~uint16_t(1 << ann.announcement_type);

        if (ann.reference_type >= 1 && ann.reference_type <= 3) {
            _is_valid = size >= 7;
            if (_is_valid) {
                ann.original_network_id = GetUInt16(data);
                ann.transport_stream_id = GetUInt16(data + 2);
                ann.service_id = GetUInt16(data + 4);
                ann.component_tag = data[6];
                data += 7; size -= 7;
            }
        }

        if (_is_valid) {
            announcements.push_back(ann);
        }
    }

    // Make sure there is no truncated trailing data.
    _is_valid = _is_valid && size == 0;

    // Create additional entries for missing types.
    for (uint8_t type = 0; _is_valid && indicator != 0 && type < 16; ++type) {
        const uint16_t mask = uint16_t(1 << type);
        if ((indicator & mask) != 0) {
            indicator &= ~mask;
            announcements.push_back(Announcement(type));
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AnnouncementSupportDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 2) {
        // Get announcement_support_indicator.
        // We will check later that all annoucement types are present.
        uint16_t indicator = GetUInt16(data);
        data += 2; size -= 2;
        strm << margin << UString::Format(u"Annoucement support indicator: 0x%X", {indicator}) << std::endl;

        // List all entries.
        while (size >= 1) {
            const uint8_t type = (data[0] >> 4) & 0x0F;
            const uint8_t ref = data[0] & 0x07;
            data++; size--;

            // Clear types one by one in announcement_support_indicator.
            indicator &= ~uint16_t(1 << type);

            strm << margin << "- Announcement type: " << NameFromSection(u"AnnouncementType", type, names::DECIMAL_FIRST) << std::endl
                 << margin << "  Reference type: " << NameFromSection(u"AnnouncementReferenceType", ref, names::DECIMAL_FIRST) << std::endl;
            if (ref >= 1 && ref <= 3) {
                if (size < 7) {
                    break;
                }
                strm << margin << UString::Format(u"  Original network id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
                     << margin << UString::Format(u"  Transport stream id: 0x%X (%d)", {GetUInt16(data + 2), GetUInt16(data + 2)}) << std::endl
                     << margin << UString::Format(u"  Service id: 0x%X (%d)", {GetUInt16(data + 4), GetUInt16(data + 4)}) << std::endl
                     << margin << UString::Format(u"  Component tag: 0x%X (%d)", {data[6], data[6]}) << std::endl;
                data += 7; size -= 7;
            }
        }

        // List missing types.
        for (uint8_t type = 0; indicator != 0 && type < 16; ++type) {
            const uint16_t mask = uint16_t(1 << type);
            if ((indicator & mask) != 0) {
                indicator &= ~mask;
                strm << margin << "- Missing announcement type: " << NameFromSection(u"AnnouncementType", type, names::DECIMAL_FIRST) << std::endl;
            }
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AnnouncementSupportDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto it = announcements.begin(); it != announcements.end(); ++it) {
        xml::Element* e = root->addElement(u"announcement");
        e->setIntAttribute(u"announcement_type", it->announcement_type);
        e->setIntAttribute(u"reference_type", it->reference_type);
        if (it->reference_type >= 1 && it->reference_type <= 3) {
            e->setIntAttribute(u"original_network_id", it->original_network_id, true);
            e->setIntAttribute(u"transport_stream_id", it->transport_stream_id, true);
            e->setIntAttribute(u"service_id", it->service_id, true);
            e->setIntAttribute(u"component_tag", it->component_tag, true);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AnnouncementSupportDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xann;
    bool ok = element->getChildren(xann, u"announcement");

    for (size_t i = 0; ok && i < xann.size(); ++i) {
        Announcement ann;
        ok = xann[i]->getIntAttribute<uint8_t>(ann.announcement_type, u"announcement_type", true, 0, 0x00, 0x0F) &&
             xann[i]->getIntAttribute<uint8_t>(ann.reference_type, u"reference_type", true, 0, 0x00, 0x07) &&
             xann[i]->getIntAttribute<uint16_t>(ann.original_network_id, u"original_network_id", ann.reference_type >= 1 && ann.reference_type <= 3) &&
             xann[i]->getIntAttribute<uint16_t>(ann.transport_stream_id, u"transport_stream_id", ann.reference_type >= 1 && ann.reference_type <= 3) &&
             xann[i]->getIntAttribute<uint16_t>(ann.service_id, u"service_id", ann.reference_type >= 1 && ann.reference_type <= 3) &&
             xann[i]->getIntAttribute<uint8_t>(ann.component_tag, u"component_tag", ann.reference_type >= 1 && ann.reference_type <= 3);
        if (ok) {
            announcements.push_back(ann);
        }
    }
    return ok;
}
