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

#include "tsServiceGroupDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsMJD.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"service_group_descriptor"
#define MY_CLASS ts::ServiceGroupDescriptor
#define MY_DID ts::DID_ISDB_SERVICE_GROUP
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ServiceGroupDescriptor::ServiceGroupDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    service_group_type(0),
    simultaneous_services(),
    private_data()
{
}

void ts::ServiceGroupDescriptor::clearContent()
{
    service_group_type = 0;
    simultaneous_services.clear();
    private_data.clear();
}

ts::ServiceGroupDescriptor::ServiceGroupDescriptor(DuckContext& duck, const Descriptor& desc) :
    ServiceGroupDescriptor()
{
    deserialize(duck, desc);
}

ts::ServiceGroupDescriptor::SimultaneousService::SimultaneousService() :
    primary_service_id(0),
    secondary_service_id(0)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ServiceGroupDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(service_group_type << 4) | 0x0F);
    if (service_group_type == 1) {
        for (auto it = simultaneous_services.begin(); it != simultaneous_services.end(); ++it) {
            bbp->appendUInt16(it->primary_service_id);
            bbp->appendUInt16(it->secondary_service_id);
        }
    }
    else {
        bbp->append(private_data);
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ServiceGroupDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1;

    simultaneous_services.clear();
    private_data.clear();

    if (_is_valid) {
        service_group_type = (data[0] >> 4) & 0x0F;
        data++; size--;

        if (service_group_type == 1) {
            while (size >= 4) {
                SimultaneousService ss;
                ss.primary_service_id = GetUInt16(data);
                ss.secondary_service_id = GetUInt16(data + 2);
                simultaneous_services.push_back(ss);
                data += 4; size -= 4;
            }
            _is_valid = size == 0;
        }
        else {
            private_data.copy(data, size);
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ServiceGroupDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        const uint8_t type = (data[0] >> 4) & 0x0F;
        data++; size--;
        strm << margin << "Group type: " << NameFromSection(u"ISDBServiceGroupType", type, names::DECIMAL_FIRST) << std::endl;

        if (type == 1) {
            strm << margin << "Simultaneous services:" << (size < 4 ? " none" : "") << std::endl;
            while (size >= 4) {
                strm << margin << UString::Format(u"- Primary service id:   0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
                     << margin << UString::Format(u"  Secondary service id: 0x%X (%d)", {GetUInt16(data + 2), GetUInt16(data + 2)}) << std::endl;
                data += 4; size -= 4;
            }
            display.displayExtraData(data, size, indent);
        }
        else {
            display.displayPrivateData(u"Private data", data, size, indent);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ServiceGroupDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"service_group_type", service_group_type);
    if (service_group_type == 1) {
        for (auto it = simultaneous_services.begin(); it != simultaneous_services.end(); ++it) {
            xml::Element* e = root->addElement(u"service");
            e->setIntAttribute(u"primary_service_id", it->primary_service_id, true);
            e->setIntAttribute(u"secondary_service_id", it->secondary_service_id, true);
        }
    }
    else {
        root->addHexaTextChild(u"private_data", private_data, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ServiceGroupDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xserv;
    bool ok =
        element->getIntAttribute<uint8_t>(service_group_type, u"service_group_type", true, 0, 0, 15) &&
        element->getChildren(xserv, u"service", 0, service_group_type == 1 ? 63 : 0) &&
        element->getHexaTextChild(private_data, u"private_data", false, 0, service_group_type == 1 ? 0 : 254);

    for (auto it = xserv.begin(); ok && it != xserv.end(); ++it) {
        SimultaneousService ss;
        ok = (*it)->getIntAttribute<uint16_t>(ss.primary_service_id, u"primary_service_id", true) &&
             (*it)->getIntAttribute<uint16_t>(ss.secondary_service_id, u"secondary_service_id", true);
        simultaneous_services.push_back(ss);
    }
    return ok;
}
