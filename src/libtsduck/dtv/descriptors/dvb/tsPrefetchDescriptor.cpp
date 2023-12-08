//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPrefetchDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"prefetch_descriptor"
#define MY_CLASS ts::PrefetchDescriptor
#define MY_DID ts::DID_AIT_PREFETCH
#define MY_TID ts::TID_AIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::PrefetchDescriptor::PrefetchDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::PrefetchDescriptor::clearContent()
{
    transport_protocol_label = 0;
    entries.clear();
}

ts::PrefetchDescriptor::PrefetchDescriptor(DuckContext& duck, const Descriptor& desc) :
    PrefetchDescriptor()
{
    deserialize(duck, desc);
}

ts::PrefetchDescriptor::Entry::Entry(const UString& str, uint8_t pri) :
    label(str),
    prefetch_priority(pri)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::PrefetchDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(transport_protocol_label);
    for (const auto& it : entries) {
        buf.putStringWithByteLength(it.label);
        buf.putUInt8(it.prefetch_priority);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::PrefetchDescriptor::deserializePayload(PSIBuffer& buf)
{
    transport_protocol_label = buf.getUInt8();
    while (buf.canRead()) {
        Entry e;
        buf.getStringWithByteLength(e.label);
        e.prefetch_priority = buf.getUInt8();
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::PrefetchDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << UString::Format(u"Transport protocol label: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        while (buf.canReadBytes(1)) {
            disp << margin << "Label: \"" << buf.getStringWithByteLength() << "\"";
            if (buf.canReadBytes(1)) {
                disp << UString::Format(u", prefetch priority: %d", {buf.getUInt8()});
            }
            disp << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::PrefetchDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"transport_protocol_label", transport_protocol_label, true);
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"module");
        e->setAttribute(u"label", it.label);
        e->setIntAttribute(u"prefetch_priority", it.prefetch_priority, false);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::PrefetchDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute(transport_protocol_label, u"transport_protocol_label", true) &&
        element->getChildren(children, u"module");

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getAttribute(entry.label, u"label", true) &&
             children[i]->getIntAttribute(entry.prefetch_priority, u"prefetch_priority", true, 1, 1, 100);
        entries.push_back(entry);
    }
    return ok;
}
