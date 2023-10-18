//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsServiceGroupDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsMJD.h"

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ServiceGroupDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(service_group_type, 4);
    buf.putBits(0xFF, 4);
    if (service_group_type == 1) {
        for (const auto& it : simultaneous_services) {
            buf.putUInt16(it.primary_service_id);
            buf.putUInt16(it.secondary_service_id);
        }
    }
    else {
        buf.putBytes(private_data);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ServiceGroupDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(service_group_type, 4);
    buf.skipBits(4);
    if (service_group_type == 1) {
        while (buf.canRead()) {
            SimultaneousService ss;
            ss.primary_service_id = buf.getUInt16();
            ss.secondary_service_id = buf.getUInt16();
            simultaneous_services.push_back(ss);
        }
    }
    else {
        buf.getBytes(private_data);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ServiceGroupDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        const uint8_t type = buf.getBits<uint8_t>(4);
        buf.skipBits(4);
        disp << margin << "Group type: " << DataName(MY_XML_NAME, u"Type", type, NamesFlags::DECIMAL_FIRST) << std::endl;
        if (type == 1) {
            disp << margin << "Simultaneous services:" << (buf.canRead() ? "" : " none") << std::endl;
            while (buf.canReadBytes(4)) {
                disp << margin << UString::Format(u"- Primary service id:   0x%X (%<d)", {buf.getUInt16()}) << std::endl;
                disp << margin << UString::Format(u"  Secondary service id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
            }
        }
        else {
            disp.displayPrivateData(u"Private data", buf, NPOS, margin);
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
        for (const auto& it : simultaneous_services) {
            xml::Element* e = root->addElement(u"service");
            e->setIntAttribute(u"primary_service_id", it.primary_service_id, true);
            e->setIntAttribute(u"secondary_service_id", it.secondary_service_id, true);
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
        element->getIntAttribute(service_group_type, u"service_group_type", true, 0, 0, 15) &&
        element->getChildren(xserv, u"service", 0, service_group_type == 1 ? 63 : 0) &&
        element->getHexaTextChild(private_data, u"private_data", false, 0, service_group_type == 1 ? 0 : 254);

    for (auto it = xserv.begin(); ok && it != xserv.end(); ++it) {
        SimultaneousService ss;
        ok = (*it)->getIntAttribute(ss.primary_service_id, u"primary_service_id", true) &&
             (*it)->getIntAttribute(ss.secondary_service_id, u"secondary_service_id", true);
        simultaneous_services.push_back(ss);
    }
    return ok;
}
