//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//
//  Representation of a logical_channel_number_descriptor.
//  Private descriptor, must be preceeded by the EACEM/EICTA PDS.
//
//----------------------------------------------------------------------------

#include "tsEutelsatChannelNumberDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"eutelsat_channel_number_descriptor"
#define MY_DID ts::DID_EUTELSAT_CHAN_NUM
#define MY_PDS ts::PDS_EUTELSAT
#define MY_STD ts::STD_DVB

TS_XML_DESCRIPTOR_FACTORY(ts::EutelsatChannelNumberDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::EutelsatChannelNumberDescriptor, ts::EDID::Private(MY_DID, MY_PDS));
TS_FACTORY_REGISTER(ts::EutelsatChannelNumberDescriptor::DisplayDescriptor, ts::EDID::Private(MY_DID, MY_PDS));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::EutelsatChannelNumberDescriptor::EutelsatChannelNumberDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS),
    entries()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::EutelsatChannelNumberDescriptor::EutelsatChannelNumberDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS),
    entries()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::EutelsatChannelNumberDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(new ByteBlock(2));
    CheckNonNull(bbp.pointer());

    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        bbp->appendUInt16(it->onetw_id);
        bbp->appendUInt16(it->ts_id);
        bbp->appendUInt16(it->service_id);
        bbp->appendUInt16(0xF000 | (it->ecn & 0x0FFF));
    }

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d(bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::EutelsatChannelNumberDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() % 8 == 0;
    entries.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        while (size >= 8) {
            entries.push_back(Entry(GetUInt16(data), GetUInt16(data + 2), GetUInt16(data + 4), GetUInt16(data + 6) & 0x0FFF));
            data += 8;
            size -= 8;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::EutelsatChannelNumberDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.duck().out());
    const std::string margin(indent, ' ');

    while (size >= 8) {
        const uint16_t onetw_id = GetUInt16(data);
        const uint16_t ts_id = GetUInt16(data + 2);
        const uint16_t service_id = GetUInt16(data + 4);
        const uint16_t channel = GetUInt16(data + 6) & 0x0FFF;
        data += 8; size -= 8;
        strm << margin
             << UString::Format(u"Service Id: %5d (0x%04X), Channel number: %3d, TS Id: %5d (0x%04X), Net Id: %5d (0x%04X)",
                                {service_id, service_id, channel, ts_id, ts_id, onetw_id, onetw_id})
             << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::EutelsatChannelNumberDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"original_network_id", it->onetw_id, true);
        e->setIntAttribute(u"transport_stream_id", it->ts_id, true);
        e->setIntAttribute(u"service_id", it->service_id, true);
        e->setIntAttribute(u"eutelsat_channel_number", it->ecn, false);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::EutelsatChannelNumberDescriptor::fromXML(DuckContext& duck, const xml::Element* element)
{
    entries.clear();

    xml::ElementVector children;
    _is_valid =
        checkXMLName(element) &&
        element->getChildren(children, u"service", 0, MAX_ENTRIES);

    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        Entry entry;
        _is_valid =
            children[i]->getIntAttribute<uint16_t>(entry.onetw_id, u"original_network_id", true, 0, 0x0000, 0xFFFF) &&
            children[i]->getIntAttribute<uint16_t>(entry.ts_id, u"transport_stream_id", true, 0, 0x0000, 0xFFFF) &&
            children[i]->getIntAttribute<uint16_t>(entry.service_id, u"service_id", true, 0, 0x0000, 0xFFFF) &&
            children[i]->getIntAttribute<uint16_t>(entry.ecn, u"eutelsat_channel_number", true, 0, 0x0000, 0x03FF);
        if (_is_valid) {
            entries.push_back(entry);
        }
    }
}
