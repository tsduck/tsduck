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

#include "tsNorDigLogicalChannelDescriptorV2.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"nordig_logical_channel_descriptor_v2"
#define MY_CLASS ts::NorDigLogicalChannelDescriptorV2
#define MY_DID ts::DID_NORDIG_CHAN_NUM_V2
#define MY_PDS ts::PDS_NORDIG
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::NorDigLogicalChannelDescriptorV2::NorDigLogicalChannelDescriptorV2() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS),
    entries()
{
}

ts::NorDigLogicalChannelDescriptorV2::NorDigLogicalChannelDescriptorV2(DuckContext& duck, const Descriptor& desc) :
    NorDigLogicalChannelDescriptorV2()
{
    deserialize(duck, desc);
}

ts::NorDigLogicalChannelDescriptorV2::Service::Service(uint16_t id, bool vis, uint16_t num):
    service_id(id),
    visible(vis),
    lcn(num)
{
}

ts::NorDigLogicalChannelDescriptorV2::ChannelList::ChannelList(uint8_t id, const UString& name, const UString& country) :
    channel_list_id(id),
    channel_list_name(name),
    country_code(country),
    services()
{
}

void ts::NorDigLogicalChannelDescriptorV2::clearContent()
{
    entries.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::NorDigLogicalChannelDescriptorV2::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    for (auto it1 = entries.begin(); it1 != entries.end(); ++it1) {
        bbp->appendUInt8(it1->channel_list_id);
        bbp->append(duck.encodedWithByteLength(it1->channel_list_name));
        if (!SerializeLanguageCode(*bbp, it1->country_code)) {
            desc.invalidate();
            return;
        }
        bbp->appendUInt8(uint8_t(it1->services.size() * 4));
        for (auto it2 = it1->services.begin(); it2 != it1->services.end(); ++it2) {
            bbp->appendUInt16(it2->service_id);
            bbp->appendUInt16((it2->visible ? 0xFC00 : 0x7C00) | (it2->lcn & 0x03FF));
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::NorDigLogicalChannelDescriptorV2::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag();
    entries.clear();

    while (_is_valid && size >= 2) {
        ChannelList clist(data[0]);
        data++; size--;
        duck.decodeWithByteLength(clist.channel_list_name, data, size);
        _is_valid = size >= 4;
        if (_is_valid) {
            clist.country_code = DeserializeLanguageCode(data);
            size_t len = data[3];
            data += 4; size -= 4;
            while (len >= 4 && size >= 4) {
                clist.services.push_back(Service(GetUInt16(data), (data[2] & 0x80) != 0, GetUInt16(data + 2) & 0x03FF));
                data += 4; size -= 4; len -= 4;
            }
            _is_valid = len == 0;
            entries.push_back(clist);
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::NorDigLogicalChannelDescriptorV2::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    while (size >= 2) {
        const uint8_t id = data[0];
        data++; size--;
        const UString name(duck.decodedWithByteLength(data, size));
        strm << margin << UString::Format(u"- Channel list id: 0x%X (%d), name: \"%s\"", {id, id, name});
        if (size < 3) {
            strm << std::endl;
            break;
        }
        strm << ", country code: \"" << DeserializeLanguageCode(data) << "\"" << std::endl;
        data += 3; size -= 3;
        if (size < 1) {
            break;
        }
        size_t len = data[0];
        data++; size--;
        while (len >= 4 && size >= 4) {
            const uint16_t service = GetUInt16(data);
            const uint8_t visible = (data[2] >> 7) & 0x01;
            const uint16_t channel = GetUInt16(data + 2) & 0x03FF;
            strm << margin
                 << UString::Format(u"  Service Id: %5d (0x%04X), Visible: %1d, Channel number: %3d", {service, service, visible, channel})
                 << std::endl;
            data += 4; size -= 4; len -= 4;
        }
        if (len != 0) {
            break;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::NorDigLogicalChannelDescriptorV2::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto it1 = entries.begin(); it1 != entries.end(); ++it1) {
        xml::Element* e1 = root->addElement(u"channel_list");
        e1->setIntAttribute(u"id", it1->channel_list_id, true);
        e1->setAttribute(u"name", it1->channel_list_name);
        e1->setAttribute(u"country_code", it1->country_code);
        for (auto it2 = it1->services.begin(); it2 != it1->services.end(); ++it2) {
            xml::Element* e2 = e1->addElement(u"service");
            e2->setIntAttribute(u"service_id", it2->service_id, true);
            e2->setIntAttribute(u"logical_channel_number", it2->lcn, false);
            e2->setBoolAttribute(u"visible_service", it2->visible);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::NorDigLogicalChannelDescriptorV2::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xclists;
    bool ok = element->getChildren(xclists, u"channel_list");

    for (size_t i1 = 0; ok && i1 < xclists.size(); ++i1) {
        ChannelList clist;
        xml::ElementVector xsrv;
        ok = xclists[i1]->getIntAttribute<uint8_t>(clist.channel_list_id, u"id", true) &&
             xclists[i1]->getAttribute(clist.channel_list_name, u"name", true) &&
             xclists[i1]->getAttribute(clist.country_code, u"country_code", true, UString(), 3, 3) &&
             xclists[i1]->getChildren(xsrv, u"service");
        for (size_t i2 = 0; ok && i2 < xsrv.size(); ++i2) {
            Service srv;
            ok = xsrv[i2]->getIntAttribute<uint16_t>(srv.service_id, u"service_id", true) &&
                 xsrv[i2]->getIntAttribute<uint16_t>(srv.lcn, u"logical_channel_number", true, 0, 0x0000, 0x03FF) &&
                 xsrv[i2]->getBoolAttribute(srv.visible, u"visible_service", false, true);
            clist.services.push_back(srv);
        }
        entries.push_back(clist);
    }
    return ok;
}
