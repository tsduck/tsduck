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

#include "tsCAServiceDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"CA_service_descriptor"
#define MY_CLASS ts::CAServiceDescriptor
#define MY_DID ts::DID_ISDB_CA_SERVICE
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CAServiceDescriptor::CAServiceDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    CA_system_id(0),
    ca_broadcaster_group_id(0),
    message_control(0),
    service_ids()
{
}

void ts::CAServiceDescriptor::clearContent()
{
    CA_system_id = 0;
    ca_broadcaster_group_id = 0;
    message_control = 0;
    service_ids.clear();
}

ts::CAServiceDescriptor::CAServiceDescriptor(DuckContext& duck, const Descriptor& desc) :
    CAServiceDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CAServiceDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt16(CA_system_id);
    bbp->appendUInt8(ca_broadcaster_group_id);
    bbp->appendUInt8(message_control);
    for (auto it = service_ids.begin(); it != service_ids.end(); ++it) {
        bbp->appendUInt16(*it);
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CAServiceDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    service_ids.clear();
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 4 && size % 2 == 0;

    if (_is_valid) {
        CA_system_id = GetUInt16(data);
        ca_broadcaster_group_id = data[2];
        message_control = data[3];
        data += 4; size -= 4;
        while (size >= 2) {
            service_ids.push_back(GetUInt16(data));
            data += 2; size -= 2;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CAServiceDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 4) {
        strm << margin << "CA System Id: " << names::CASId(duck, GetUInt16(data), names::FIRST) << std::endl
             << margin << UString::Format(u"CA broadcaster group id: 0x%X (%d)", {data[2], data[2]}) << std::endl
             << margin << UString::Format(u"Delay time: %d days", {data[3]}) << std::endl;
        data += 4; size -= 4;
        while (size >= 2) {
            strm << margin << UString::Format(u"Service id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl;
            data += 2; size -= 2;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CAServiceDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"CA_system_id", CA_system_id, true);
    root->setIntAttribute(u"ca_broadcaster_group_id", ca_broadcaster_group_id, true);
    root->setIntAttribute(u"message_control", message_control);
    for (auto it = service_ids.begin(); it != service_ids.end(); ++it) {
        root->addElement(u"service")->setIntAttribute(u"id", *it, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::CAServiceDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xserv;
    bool ok =
        element->getIntAttribute<uint16_t>(CA_system_id, u"CA_system_id", true) &&
        element->getIntAttribute<uint8_t>(ca_broadcaster_group_id, u"ca_broadcaster_group_id", true) &&
        element->getIntAttribute<uint8_t>(message_control, u"message_control", true) &&
        element->getChildren(xserv, u"service", 0, 125);

    for (auto it = xserv.begin(); ok && it != xserv.end(); ++it) {
        uint16_t id = 0;
        ok = (*it)->getIntAttribute<uint16_t>(id, u"id", true);
        service_ids.push_back(id);
    }
    return ok;
}
