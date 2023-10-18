//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNorDigLogicalChannelDescriptorV2.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS)
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
    country_code(country)
{
}

void ts::NorDigLogicalChannelDescriptorV2::clearContent()
{
    entries.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::NorDigLogicalChannelDescriptorV2::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it1 : entries) {
        buf.putUInt8(it1.channel_list_id);
        buf.putStringWithByteLength(it1.channel_list_name);
        buf.putLanguageCode(it1.country_code);
        buf.pushWriteSequenceWithLeadingLength(8); // descriptor_length
        for (const auto& it2 : it1.services) {
            buf.putUInt16(it2.service_id);
            buf.putBit(it2.visible);
            buf.putBits(0xFF, 5);
            buf.putBits(it2.lcn, 10);
        }
        buf.popState(); // update descriptor_length
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::NorDigLogicalChannelDescriptorV2::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        ChannelList clist(buf.getUInt8());
        buf.getStringWithByteLength(clist.channel_list_name);
        buf.getLanguageCode(clist.country_code);
        buf.pushReadSizeFromLength(8); // descriptor_length
        while (buf.canRead()) {
            Service srv;
            srv.service_id = buf.getUInt16();
            srv.visible = buf.getBool();
            buf.skipBits(5);
            buf.getBits(srv.lcn, 10);
            clist.services.push_back(srv);
        }
        buf.popState(); // descriptor_length
        entries.push_back(clist);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::NorDigLogicalChannelDescriptorV2::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"- Channel list id: 0x%X (%<d)", {buf.getUInt8()});
        disp << ", name: \"" << buf.getStringWithByteLength() << "\"";
        if (!buf.canReadBytes(3)) {
            disp << std::endl;
            break;
        }
        disp << ", country code: \"" << buf.getLanguageCode() << "\"" << std::endl;
        buf.pushReadSizeFromLength(8); // descriptor_length
        while (buf.canReadBytes(4)) {
            disp << margin << UString::Format(u"  Service Id: %5d (0x%<04X)", {buf.getUInt16()});
            disp << UString::Format(u", Visible: %1d", {buf.getBit()});
            buf.skipBits(5);
            disp << UString::Format(u", Channel number: %3d", {buf.getBits<uint16_t>(10)}) << std::endl;
        }
        buf.popState(); // descriptor_length
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::NorDigLogicalChannelDescriptorV2::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it1 : entries) {
        xml::Element* e1 = root->addElement(u"channel_list");
        e1->setIntAttribute(u"id", it1.channel_list_id, true);
        e1->setAttribute(u"name", it1.channel_list_name);
        e1->setAttribute(u"country_code", it1.country_code);
        for (const auto& it2 : it1.services) {
            xml::Element* e2 = e1->addElement(u"service");
            e2->setIntAttribute(u"service_id", it2.service_id, true);
            e2->setIntAttribute(u"logical_channel_number", it2.lcn, false);
            e2->setBoolAttribute(u"visible_service", it2.visible);
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
        ok = xclists[i1]->getIntAttribute(clist.channel_list_id, u"id", true) &&
             xclists[i1]->getAttribute(clist.channel_list_name, u"name", true) &&
             xclists[i1]->getAttribute(clist.country_code, u"country_code", true, UString(), 3, 3) &&
             xclists[i1]->getChildren(xsrv, u"service");
        for (size_t i2 = 0; ok && i2 < xsrv.size(); ++i2) {
            Service srv;
            ok = xsrv[i2]->getIntAttribute(srv.service_id, u"service_id", true) &&
                 xsrv[i2]->getIntAttribute(srv.lcn, u"logical_channel_number", true, 0, 0x0000, 0x03FF) &&
                 xsrv[i2]->getBoolAttribute(srv.visible, u"visible_service", false, true);
            clist.services.push_back(srv);
        }
        entries.push_back(clist);
    }
    return ok;
}
