//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSInformationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"TS_information_descriptor"
#define MY_CLASS ts::TSInformationDescriptor
#define MY_DID ts::DID_ISDB_TS_INFO
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TSInformationDescriptor::TSInformationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::TSInformationDescriptor::clearContent()
{
    remote_control_key_id = 0;
    ts_name.clear();
    transmission_types.clear();
    reserved_future_use.clear();
}

ts::TSInformationDescriptor::TSInformationDescriptor(DuckContext& duck, const Descriptor& desc) :
    TSInformationDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TSInformationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(remote_control_key_id);

    buf.pushState();    // save position of length_of_ts_name
    buf.putBits(0, 6);  // placeholder for length_of_ts_name
    buf.putBits(transmission_types.size(), 2);
    const size_t start = buf.currentWriteByteOffset();
    buf.putString(ts_name);
    const size_t length_of_ts_name = buf.currentWriteByteOffset() - start;
    buf.swapState();    // move back at length_of_ts_name
    buf.putBits(length_of_ts_name, 6);
    buf.popState();     // move at current end of descriptor

    for (const auto& it1 : transmission_types) {
        buf.putUInt8(it1.transmission_type_info);
        buf.putUInt8(uint8_t(it1.service_ids.size()));
        for (auto it2 : it1.service_ids) {
            buf.putUInt16(it2);
        }
    }
    buf.putBytes(reserved_future_use);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TSInformationDescriptor::deserializePayload(PSIBuffer& buf)
{
    remote_control_key_id = buf.getUInt8();
    const size_t nlen = buf.getBits<size_t>(6);
    const size_t tcount = buf.getBits<size_t>(2);
    buf.getString(ts_name, nlen);
    for (size_t i1 = 0; i1 < tcount && !buf.error(); ++i1) {
        Entry e;
        e.transmission_type_info = buf.getUInt8();
        const size_t scount = buf.getUInt8();
        for (size_t i2 = 0; i2 < scount && !buf.error(); ++i2) {
            e.service_ids.push_back(buf.getUInt16());
        }
        transmission_types.push_back(e);
    }
    buf.getBytes(reserved_future_use);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TSInformationDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"Remote control key id: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        const size_t nlen = buf.getBits<size_t>(6);
        const size_t tcount = buf.getBits<size_t>(2);
        disp << margin << "TS name: \"" << buf.getString(nlen) << "\"" << std::endl;

        for (size_t i1 = 0; buf.canReadBytes(2) && i1 < tcount; ++i1) {
            disp << margin << UString::Format(u"- Transmission type info: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
            const size_t scount = buf.getUInt8();
            for (size_t i2 = 0; buf.canReadBytes(2) && i2 < scount; ++i2) {
                disp << margin << UString::Format(u"  Service id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            }
        }
        disp.displayPrivateData(u"Reserved for future use", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TSInformationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"remote_control_key_id", remote_control_key_id, true);
    root->setAttribute(u"ts_name", ts_name);
    for (const auto& it1 : transmission_types) {
        xml::Element* e = root->addElement(u"transmission_type");
        e->setIntAttribute(u"transmission_type_info", it1.transmission_type_info, true);
        for (auto it2 : it1.service_ids) {
            e->addElement(u"service")->setIntAttribute(u"id", it2, true);
        }
    }
    root->addHexaTextChild(u"reserved_future_use", reserved_future_use, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TSInformationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xtype;
    bool ok =
        element->getIntAttribute(remote_control_key_id, u"remote_control_key_id", true) &&
        element->getAttribute(ts_name, u"ts_name", true) &&
        element->getHexaTextChild(reserved_future_use, u"reserved_future_use") &&
        element->getChildren(xtype, u"transmission_type", 0, 3);

    for (auto it1 = xtype.begin(); ok && it1 != xtype.end(); ++it1) {
        Entry e;
        xml::ElementVector xserv;
        ok = (*it1)->getIntAttribute(e.transmission_type_info, u"transmission_type_info", true) &&
             (*it1)->getChildren(xserv, u"service");
        for (auto it2 = xserv.begin(); ok && it2 != xserv.end(); ++it2) {
            uint16_t id = 0;
            ok = (*it2)->getIntAttribute(id, u"id", true);
            e.service_ids.push_back(id);
        }
        transmission_types.push_back(e);
    }
    return ok;
}
