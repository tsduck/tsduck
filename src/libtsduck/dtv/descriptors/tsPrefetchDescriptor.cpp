//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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

#include "tsPrefetchDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsNames.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"prefetch_descriptor"
#define MY_DID ts::DID_AIT_PREFETCH
#define MY_TID ts::TID_AIT
#define MY_STD ts::STD_DVB

TS_XML_TABSPEC_DESCRIPTOR_FACTORY(ts::PrefetchDescriptor, MY_XML_NAME, MY_TID);
TS_ID_DESCRIPTOR_FACTORY(ts::PrefetchDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));
TS_FACTORY_REGISTER(ts::PrefetchDescriptor::DisplayDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::PrefetchDescriptor::PrefetchDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    transport_protocol_label(0),
    entries()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::PrefetchDescriptor::PrefetchDescriptor(DuckContext& duck, const Descriptor& desc) :
    PrefetchDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::PrefetchDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(transport_protocol_label);
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        bbp->append(duck.toDVBWithByteLength(it->label));
        bbp->appendUInt8(it->prefetch_priority);
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::PrefetchDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    entries.clear();

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == _tag && size >= 1;

    if (_is_valid) {
        transport_protocol_label = data[0];
        data++; size--;
        while (_is_valid && size >= 1) {
            const size_t len = data[0];
            data++; size--;
            _is_valid = len + 1 <= size;
            if (_is_valid) {
                entries.push_back(Entry(duck.fromDVB(data, len), data[len]));
                data += len + 1; size -= len + 1;
            }
        }
    }

    _is_valid = _is_valid && size == 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::PrefetchDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.duck().out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        strm << margin << UString::Format(u"Transport protocol label: 0x%X (%d)", {data[0], data[0]}) << std::endl;
        data++; size--;
        while (size >= 1) {
            const size_t len = data[0];
            if (len + 2 > size) {
                break;
            }
            strm << margin
                 << UString::Format(u"Label: \"%s\", prefetch priority: %d", {display.duck().fromDVB(data + 1, len), data[len + 1]})
                 << std::endl;
            data += len + 2; size -= len + 2;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::PrefetchDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"transport_protocol_label", transport_protocol_label, true);
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"module");
        e->setAttribute(u"label", it->label);
        e->setIntAttribute(u"prefetch_priority", it->prefetch_priority, false);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::PrefetchDescriptor::fromXML(DuckContext& duck, const xml::Element* element)
{
    entries.clear();

    xml::ElementVector children;
    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint8_t>(transport_protocol_label, u"transport_protocol_label", true) &&
        element->getChildren(children, u"module");

    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        Entry entry;
        _is_valid =
            children[i]->getAttribute(entry.label, u"label", true) &&
            children[i]->getIntAttribute<uint8_t>(entry.prefetch_priority, u"prefetch_priority", true, 1, 1, 100);
        if (_is_valid) {
            entries.push_back(entry);
        }
    }
}
