//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2019-2020, Anthony Delannoy
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

#include "tsSkyLogicalChannelNumberDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"sky_logical_channel_number_descriptor"
#define MY_CLASS ts::SkyLogicalChannelNumberDescriptor
#define MY_DID ts::DID_LOGICAL_CHANNEL_SKY
#define MY_PDS ts::PDS_BSKYB
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SkyLogicalChannelNumberDescriptor::SkyLogicalChannelNumberDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS),
    entries(),
    region_id(0)
{
}

void ts::SkyLogicalChannelNumberDescriptor::clearContent()
{
    entries.clear();
    region_id = 0;
}

ts::SkyLogicalChannelNumberDescriptor::SkyLogicalChannelNumberDescriptor(DuckContext& duck, const Descriptor& desc) :
    SkyLogicalChannelNumberDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SkyLogicalChannelNumberDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt16(region_id);
    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        bbp->appendUInt16 (it->service_id);
        bbp->appendUInt8  (it->service_type);
        bbp->appendUInt16 (it->channel_id);
        bbp->appendUInt16 (it->lcn);
        bbp->appendUInt16 (it->sky_id);
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SkyLogicalChannelNumberDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() % 9 == 2;
    entries.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();

        region_id = GetUInt16(data);
        data += 2; size -= 2;

        while (size >= 9) {
            uint16_t sid = GetUInt16(data);
            uint8_t stype = GetUInt8(data + 2);
            uint16_t channel_id = GetUInt16(data + 3);
            uint16_t lcn_id = GetUInt16(data + 5);
            uint16_t sky_id = GetUInt16(data + 7);

            entries.push_back(Entry(sid, stype, channel_id, lcn_id, sky_id));
            data += 9; size -= 9;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SkyLogicalChannelNumberDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    uint16_t region_id = GetUInt16(data);
    data += 2; size -= 2;

    strm << margin
         << UString::Format(u"Region Id: %5d (0x%04X)", {region_id, region_id})
         << std::endl;

    while (size >= 9) {
        const uint16_t service_id = GetUInt16(data);
        const uint8_t service_type = GetUInt8(data + 2);
        const uint16_t channel_id = GetUInt16(data + 3);
        const uint16_t lcn_id = GetUInt16(data + 5);
        const uint16_t sky_id = GetUInt16(data + 7);
        data += 9; size -= 9;

        strm << margin
             << UString::Format(u"Service Id: %5d (0x%04X), Service Type: %s, Channel number: %3d, "
                                 "Lcn: %5d, Sky Id: %5d (0x%04X)",
                                 {service_id, service_id, names::ServiceType(service_type, names::FIRST),
                                  channel_id, lcn_id, sky_id, sky_id})
             << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SkyLogicalChannelNumberDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"region_id", region_id, true);

    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"service_id", it->service_id, true);
        e->setIntAttribute(u"service_type", it->service_type, true);
        e->setIntAttribute(u"channel_id", it->channel_id, true);
        e->setIntAttribute(u"logical_channel_number", it->lcn, false);
        e->setIntAttribute(u"sky_id", it->sky_id, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SkyLogicalChannelNumberDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute<uint16_t>(region_id, u"region_id", true, 0, 0x0000, 0xFFFF) &&
        element->getChildren(children, u"service", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getIntAttribute<uint16_t>(entry.service_id, u"service_id", true, 0, 0x0000, 0xFFFF) &&
             children[i]->getIntAttribute<uint8_t>(entry.service_type, u"service_type", true, 0, 0x00, 0xFF) &&
             children[i]->getIntAttribute<uint16_t>(entry.channel_id, u"channel_id", true, 0, 0x0000, 0xFFFF) &&
             children[i]->getIntAttribute<uint16_t>(entry.lcn, u"logical_channel_number", true, 0, 0x0000, 0xFFFF) &&
             children[i]->getIntAttribute<uint16_t>(entry.sky_id, u"sky_id", true, 0, 0x0000, 0xFFFF);
        entries.push_back(entry);
    }
    return ok;
}
