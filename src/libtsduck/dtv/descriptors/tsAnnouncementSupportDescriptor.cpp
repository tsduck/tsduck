//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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

void ts::AnnouncementSupportDescriptor::serializePayload(PSIBuffer& buf) const
{
    // Rebuild announcement_support_indicator
    uint16_t indicator = 0;
    for (const auto& it : announcements) {
        indicator |= uint16_t(1 << it.announcement_type);
    }

    buf.putUInt16(indicator);
    for (const auto& it : announcements) {
        buf.putBits(it.announcement_type, 4);
        buf.putBit(1);
        buf.putBits(it.reference_type, 3);
        if (it.reference_type >= 1 && it.reference_type <= 3) {
            buf.putUInt16(it.original_network_id);
            buf.putUInt16(it.transport_stream_id);
            buf.putUInt16(it.service_id);
            buf.putUInt8(it.component_tag);
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AnnouncementSupportDescriptor::deserializePayload(PSIBuffer& buf)
{
    // Get announcement_support_indicator.
    // We will check later that all annoucement types are present.
    uint16_t indicator = buf.getUInt16();

    while (buf.canRead()) {
        Announcement ann;
        buf.getBits(ann.announcement_type, 4);
        buf.skipBits(1);
        buf.getBits(ann.reference_type, 3);

        // Clear types one by one in announcement_support_indicator.
        indicator &= ~uint16_t(1 << ann.announcement_type);

        if (ann.reference_type >= 1 && ann.reference_type <= 3) {
            ann.original_network_id = buf.getUInt16();
            ann.transport_stream_id = buf.getUInt16();
            ann.service_id = buf.getUInt16();
            ann.component_tag = buf.getUInt8();
        }
        announcements.push_back(ann);
    }

    // Create additional entries for missing types.
    for (uint8_t type = 0; indicator != 0 && type < 16; ++type) {
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

void ts::AnnouncementSupportDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        // Get announcement_support_indicator.
        // We will check later that all annoucement types are present.
        uint16_t indicator = buf.getUInt16();
        disp << margin << UString::Format(u"Annoucement support indicator: 0x%X", {indicator}) << std::endl;

        // List all entries.
        while (buf.canReadBytes(1)) {
            const uint8_t type = buf.getBits<uint8_t>(4);
            buf.skipBits(1);
            const uint8_t ref = buf.getBits<uint8_t>(3);

            // Clear types one by one in announcement_support_indicator.
            indicator &= ~uint16_t(1 << type);

            disp << margin << "- Announcement type: " << DataName(MY_XML_NAME, u"Type", type, NamesFlags::DECIMAL_FIRST) << std::endl;
            disp << margin << "  Reference type: " << DataName(MY_XML_NAME, u"ReferenceType", ref, NamesFlags::DECIMAL_FIRST) << std::endl;
            if (ref >= 1 && ref <= 3 && buf.canReadBytes(7)) {
                disp << margin << UString::Format(u"  Original network id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                disp << margin << UString::Format(u"  Transport stream id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                disp << margin << UString::Format(u"  Service id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                disp << margin << UString::Format(u"  Component tag: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
            }
        }

        // List missing types.
        for (uint8_t type = 0; indicator != 0 && type < 16; ++type) {
            const uint16_t mask = uint16_t(1 << type);
            if ((indicator & mask) != 0) {
                indicator &= ~mask;
                disp << margin << "- Missing announcement type: " << DataName(MY_XML_NAME, u"Type", type, NamesFlags::DECIMAL_FIRST) << std::endl;
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AnnouncementSupportDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : announcements) {
        xml::Element* e = root->addElement(u"announcement");
        e->setIntAttribute(u"announcement_type", it.announcement_type);
        e->setIntAttribute(u"reference_type", it.reference_type);
        if (it.reference_type >= 1 && it.reference_type <= 3) {
            e->setIntAttribute(u"original_network_id", it.original_network_id, true);
            e->setIntAttribute(u"transport_stream_id", it.transport_stream_id, true);
            e->setIntAttribute(u"service_id", it.service_id, true);
            e->setIntAttribute(u"component_tag", it.component_tag, true);
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
        ok = xann[i]->getIntAttribute(ann.announcement_type, u"announcement_type", true, 0, 0x00, 0x0F) &&
             xann[i]->getIntAttribute(ann.reference_type, u"reference_type", true, 0, 0x00, 0x07) &&
             xann[i]->getIntAttribute(ann.original_network_id, u"original_network_id", ann.reference_type >= 1 && ann.reference_type <= 3) &&
             xann[i]->getIntAttribute(ann.transport_stream_id, u"transport_stream_id", ann.reference_type >= 1 && ann.reference_type <= 3) &&
             xann[i]->getIntAttribute(ann.service_id, u"service_id", ann.reference_type >= 1 && ann.reference_type <= 3) &&
             xann[i]->getIntAttribute(ann.component_tag, u"component_tag", ann.reference_type >= 1 && ann.reference_type <= 3);
        if (ok) {
            announcements.push_back(ann);
        }
    }
    return ok;
}
