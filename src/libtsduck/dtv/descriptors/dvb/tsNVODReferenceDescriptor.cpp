//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNVODReferenceDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"NVOD_reference_descriptor"
#define MY_CLASS    ts::NVODReferenceDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_DVB_NVOD_REFERENCE, ts::Standards::DVB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::NVODReferenceDescriptor::Entry::Entry(uint16_t ts, uint16_t net, uint16_t srv) :
    transport_stream_id(ts),
    original_network_id(net),
    service_id(srv)
{
}

ts::NVODReferenceDescriptor::NVODReferenceDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::NVODReferenceDescriptor::NVODReferenceDescriptor(DuckContext& duck, const Descriptor& desc) :
    NVODReferenceDescriptor()
{
    deserialize(duck, desc);
}

void ts::NVODReferenceDescriptor::clearContent()
{
    entries.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::NVODReferenceDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : entries) {
        buf.putUInt16(it.transport_stream_id);
        buf.putUInt16(it.original_network_id);
        buf.putUInt16(it.service_id);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::NVODReferenceDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Entry e;
        e.transport_stream_id = buf.getUInt16();
        e.original_network_id = buf.getUInt16();
        e.service_id = buf.getUInt16();
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::NVODReferenceDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    while (buf.canReadBytes(6)) {
        disp << margin << UString::Format(u"- Transport stream id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"  Original network id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"  Service id: %n", buf.getUInt16()) << std::endl;
    }

}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::NVODReferenceDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"transport_stream_id", it.transport_stream_id, true);
        e->setIntAttribute(u"original_network_id", it.original_network_id, true);
        e->setIntAttribute(u"service_id", it.service_id, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::NVODReferenceDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"service", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getIntAttribute(entry.transport_stream_id, u"transport_stream_id", true) &&
             children[i]->getIntAttribute(entry.original_network_id, u"original_network_id", true) &&
             children[i]->getIntAttribute(entry.service_id, u"service_id", true);
        entries.push_back(entry);
    }
    return ok;
}
