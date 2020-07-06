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

#include "tsTSInformationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    remote_control_key_id(0),
    ts_name(),
    transmission_types(),
    reserved_future_use()
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

ts::TSInformationDescriptor::Entry::Entry() :
    transmission_type_info(0),
    service_ids()
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TSInformationDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    const ByteBlock name(duck.encoded(ts_name));
    bbp->appendUInt8(remote_control_key_id);
    bbp->appendUInt8(uint8_t(name.size() << 2) | uint8_t(transmission_types.size() & 0x03));
    bbp->append(name);
    for (auto it1 = transmission_types.begin(); it1 != transmission_types.end(); ++it1) {
        bbp->appendUInt8(it1->transmission_type_info);
        bbp->appendUInt8(uint8_t(it1->service_ids.size()));
        for (auto it2 = it1->service_ids.begin(); it2 != it1->service_ids.end(); ++it2) {
            bbp->appendUInt16(*it2);
        }
    }
    bbp->append(reserved_future_use);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TSInformationDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 2;

    ts_name.clear();
    transmission_types.clear();
    reserved_future_use.clear();

    if (_is_valid) {
        remote_control_key_id = data[0];
        const size_t nlen = (data[1] >> 2) & 0x3F;
        const size_t tcount = data[1] & 0x03;
        data += 2; size -= 2;

        if (size < nlen) {
            _is_valid = false;
            return;
        }

        duck.decode(ts_name, data, nlen);
        data += nlen; size -= nlen;

        for (size_t i1 = 0; i1 < tcount; ++i1) {
            if (size < 2) {
                _is_valid = false;
                return;
            }
            Entry e;
            e.transmission_type_info = data[0];
            const size_t scount = data[1];
            data += 2; size -= 2;
            if (size < 2 * scount) {
                _is_valid = false;
                return;
            }
            for (size_t i2 = 0; i2 < scount; ++i2) {
                e.service_ids.push_back(GetUInt16(data));
                data += 2; size -= 2;
            }
            transmission_types.push_back(e);
        }

        reserved_future_use.copy(data, size);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TSInformationDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size < 2) {
        display.displayExtraData(data, size, indent);
    }
    else {
        strm << margin << UString::Format(u"Remote control key id: 0x%X (%d)", {data[0], data[0]}) << std::endl;
        const size_t nlen = std::min<size_t>(size - 2, (data[1] >> 2) & 0x3F);
        const size_t tcount = data[1] & 0x03;
        data += 2; size -= 2;

        strm << margin << "TS name: \"" << duck.decoded(data, nlen) << "\"" << std::endl;
        data += nlen; size -= nlen;

        for (size_t i1 = 0; size >= 2 && i1 < tcount; ++i1) {
            strm << margin << UString::Format(u"- Transmission type info: 0x%X (%d)", {data[0], data[0]}) << std::endl;
            const size_t scount = data[1];
            data += 2; size -= 2;
            for (size_t i2 = 0; size >= 2 && i2 < scount; ++i2) {
                const uint16_t id = GetUInt16(data);
                strm << margin << UString::Format(u"  Service id: 0x%X (%d)", {id, id}) << std::endl;
                data += 2; size -= 2;
            }
        }

        display.displayPrivateData(u"Reserved for future use", data, size, indent);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TSInformationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"remote_control_key_id", remote_control_key_id, true);
    root->setAttribute(u"ts_name", ts_name);
    for (auto it1 = transmission_types.begin(); it1 != transmission_types.end(); ++it1) {
        xml::Element* e = root->addElement(u"transmission_type");
        e->setIntAttribute(u"transmission_type_info", it1->transmission_type_info, true);
        for (auto it2 = it1->service_ids.begin(); it2 != it1->service_ids.end(); ++it2) {
            e->addElement(u"service")->setIntAttribute(u"id", *it2, true);
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
        element->getIntAttribute<uint8_t>(remote_control_key_id, u"remote_control_key_id", true) &&
        element->getAttribute(ts_name, u"ts_name", true) &&
        element->getHexaTextChild(reserved_future_use, u"reserved_future_use") &&
        element->getChildren(xtype, u"transmission_type", 0, 3);

    for (auto it1 = xtype.begin(); ok && it1 != xtype.end(); ++it1) {
        Entry e;
        xml::ElementVector xserv;
        ok = (*it1)->getIntAttribute<uint8_t>(e.transmission_type_info, u"transmission_type_info", true) &&
             (*it1)->getChildren(xserv, u"service");
        for (auto it2 = xserv.begin(); ok && it2 != xserv.end(); ++it2) {
            uint16_t id = 0;
            ok = (*it2)->getIntAttribute<uint16_t>(id, u"id", true);
            e.service_ids.push_back(id);
        }
        transmission_types.push_back(e);
    }
    return ok;
}
