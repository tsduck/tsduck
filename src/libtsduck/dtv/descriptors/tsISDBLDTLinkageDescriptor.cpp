//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISDBLDTLinkageDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"ISDB_LDT_linkage_descriptor"
#define MY_CLASS    ts::ISDBLDTLinkageDescriptor
#define MY_DID      ts::DID_ISDB_LDT_LINKAGE
#define MY_PDS      ts::PDS_ISDB
#define MY_STD      ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ISDBLDTLinkageDescriptor::ISDBLDTLinkageDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::ISDBLDTLinkageDescriptor::ISDBLDTLinkageDescriptor(DuckContext& duck, const Descriptor& desc) :
    ISDBLDTLinkageDescriptor()
{
    deserialize(duck, desc);
}


void ts::ISDBLDTLinkageDescriptor::clearContent()
{
    original_service_id = 0;
    transport_stream_id = 0;
    original_network_id = 0;
    descriptions.clear();
}

void ts::ISDBLDTLinkageDescriptor::DescriptionType::clear()
{
    id = 0;
    type = 0;
    user_defined = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ISDBLDTLinkageDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(original_service_id);
    buf.putUInt16(transport_stream_id);
    buf.putUInt16(original_network_id);
    for (auto desc : descriptions) {
        desc.serialize(buf);
    }
}

void ts::ISDBLDTLinkageDescriptor::DescriptionType::serialize(PSIBuffer& buf) const
{
    buf.putUInt16(id);
    buf.putBits(0xFF, 4);
    buf.putBits(type, 4);
    buf.putUInt8(user_defined);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ISDBLDTLinkageDescriptor::deserializePayload(PSIBuffer& buf)
{
    original_service_id = buf.getUInt16();
    transport_stream_id = buf.getUInt16();
    original_network_id = buf.getUInt16();
    while (buf.canReadBytes(4)) {
        DescriptionType t(buf);
        descriptions.push_back(t);
    }
}

void ts::ISDBLDTLinkageDescriptor::DescriptionType::deserialize(PSIBuffer& buf)
{
    clear();
    id = buf.getUInt16();
    buf.skipBits(4);
    type = buf.getBits<uint8_t>(4);
    user_defined = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ISDBLDTLinkageDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(6)) {
        disp << margin << UString::Format(u"Original service id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Transport stream id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"Original network id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;

        while (buf.canReadBytes(3)) {
            DescriptionType desc;
            desc.display(disp, buf, margin + u" ");
        }
    }
}

void ts::ISDBLDTLinkageDescriptor::DescriptionType::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    disp << margin << "Description id: " << buf.getUInt16() << std::endl;
    buf.skipReservedBits(4);
    disp << margin << " Description type: " << DataName(MY_XML_NAME, u"description_type", buf.getBits<uint8_t>(4)) << std::endl;
    disp << margin << UString::Format(u" User defined: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ISDBLDTLinkageDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"original_service_id", original_service_id, true);
    root->setIntAttribute(u"transport_stream_id", transport_stream_id, true);
    root->setIntAttribute(u"original_network_id", original_network_id, true);
    for (auto desc : descriptions) {
        desc.toXML(root->addElement(u"Description"));
    }
}

void ts::ISDBLDTLinkageDescriptor::DescriptionType::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"id", id, true);
    root->setIntAttribute(u"type", type, true);
    root->setIntAttribute(u"user_defined", user_defined);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ISDBLDTLinkageDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector descs;
    bool ok = element->getIntAttribute(original_service_id, u"original_service_id", true) &&
              element->getIntAttribute(transport_stream_id, u"transport_stream_id", true) &&
              element->getIntAttribute(original_network_id, u"original_network_id", true) &&
              element->getChildren(descs, u"Description");

    bool descriptions_ok = true;
    if (ok) {
        for (auto desc : descs) {
            DescriptionType t;
            if (t.fromXML(desc)) {
                descriptions.push_back(t);
            }
            else {
                descriptions_ok = false;
            }
        }
    }
    return ok && descriptions_ok;
}

bool ts::ISDBLDTLinkageDescriptor::DescriptionType::fromXML(const xml::Element* element)
{
    return element->getIntAttribute(id, u"id", true) &&
           element->getIntAttribute(type, u"type", true) &&
           element->getIntAttribute(user_defined, u"user_defined", true);
}
