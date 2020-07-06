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

#include "tsEventGroupDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsMJD.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"event_group_descriptor"
#define MY_CLASS ts::EventGroupDescriptor
#define MY_DID ts::DID_ISDB_EVENT_GROUP
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::EventGroupDescriptor::EventGroupDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    group_type(0),
    actual_events(),
    other_events(),
    private_data()
{
}

ts::EventGroupDescriptor::EventGroupDescriptor(DuckContext& duck, const Descriptor& desc) :
    EventGroupDescriptor()
{
    deserialize(duck, desc);
}

void ts::EventGroupDescriptor::clearContent()
{
    group_type = 0;
    actual_events.clear();
    other_events.clear();
    private_data.clear();
}

ts::EventGroupDescriptor::ActualEvent::ActualEvent() :
    service_id(0),
    event_id(0)
{
}

ts::EventGroupDescriptor::OtherEvent::OtherEvent() :
    original_network_id(0),
    transport_stream_id(0),
    service_id(0),
    event_id(0)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::EventGroupDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(group_type << 4) | uint8_t(actual_events.size() & 0x0F));
    for (auto it = actual_events.begin(); it != actual_events.end(); ++it) {
        bbp->appendUInt16(it->service_id);
        bbp->appendUInt16(it->event_id);
    }
    if (group_type == 4 || group_type == 5) {
        for (auto it = other_events.begin(); it != other_events.end(); ++it) {
            bbp->appendUInt16(it->original_network_id);
            bbp->appendUInt16(it->transport_stream_id);
            bbp->appendUInt16(it->service_id);
            bbp->appendUInt16(it->event_id);
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

void ts::EventGroupDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1;

    actual_events.clear();
    other_events.clear();
    private_data.clear();

    if (_is_valid) {
        group_type = (data[0] >> 4) & 0x0F;
        size_t count = data[0] & 0x0F;
        data++; size--;

        while (count > 0 && size >= 4) {
            ActualEvent ev;
            ev.service_id = GetUInt16(data);
            ev.event_id = GetUInt16(data + 2);
            actual_events.push_back(ev);
            data += 4; size -= 4; count--;
        }
        _is_valid = count == 0;

        if (_is_valid) {
            if (group_type == 4 || group_type == 5) {
                while (size >= 8) {
                    OtherEvent ev;
                    ev.original_network_id = GetUInt16(data);
                    ev.transport_stream_id = GetUInt16(data + 2);
                    ev.service_id = GetUInt16(data + 4);
                    ev.event_id = GetUInt16(data + 6);
                    other_events.push_back(ev);
                    data += 8; size -= 8;
                }
                _is_valid = size == 0;
            }
            else {
                private_data.copy(data, size);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::EventGroupDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        const uint8_t type = (data[0] >> 4) & 0x0F;
        size_t count = data[0] & 0x0F;
        data++; size--;
        strm << margin << "Group type: " << NameFromSection(u"ISDBEventGroupType", type, names::DECIMAL_FIRST) << std::endl;

        strm << margin << "Actual events:" << (count == 0 ? " none" : "") << std::endl;
        while (count > 0 && size >= 4) {
            strm << margin << UString::Format(u"- Service id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
                 << margin << UString::Format(u"  Event id:   0x%X (%d)", {GetUInt16(data + 2), GetUInt16(data + 2)}) << std::endl;
            data += 4; size -= 4; count--;
        }

        if (type == 4 || type == 5) {
            strm << margin << "Other networks events:" << (size < 8 ? " none" : "") << std::endl;
            while (size >= 8) {
                strm << margin << UString::Format(u"- Original network id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
                     << margin << UString::Format(u"  Transport stream id: 0x%X (%d)", {GetUInt16(data + 2), GetUInt16(data + 2)}) << std::endl
                     << margin << UString::Format(u"  Service id:          0x%X (%d)", {GetUInt16(data + 4), GetUInt16(data + 4)}) << std::endl
                     << margin << UString::Format(u"  Event id:            0x%X (%d)", {GetUInt16(data + 6), GetUInt16(data + 6)}) << std::endl;
                data += 8; size -= 8;
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

void ts::EventGroupDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"group_type", group_type);
    for (auto it = actual_events.begin(); it != actual_events.end(); ++it) {
        xml::Element* e = root->addElement(u"actual");
        e->setIntAttribute(u"service_id", it->service_id, true);
        e->setIntAttribute(u"event_id", it->event_id, true);
    }
    if (group_type == 4 || group_type == 5) {
        for (auto it = other_events.begin(); it != other_events.end(); ++it) {
            xml::Element* e = root->addElement(u"other");
            e->setIntAttribute(u"original_network_id", it->original_network_id, true);
            e->setIntAttribute(u"transport_stream_id", it->transport_stream_id, true);
            e->setIntAttribute(u"service_id", it->service_id, true);
            e->setIntAttribute(u"event_id", it->event_id, true);
        }
    }
    else {
        root->addHexaTextChild(u"private_data", private_data, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::EventGroupDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xactual;
    xml::ElementVector xother;
    bool ok =
        element->getIntAttribute<uint8_t>(group_type, u"group_type", true, 0, 0, 15) &&
        element->getChildren(xactual, u"actual", 0, 15) &&
        element->getChildren(xother, u"other", 0, group_type == 4 || group_type == 5 ? 31 : 0) &&
        element->getHexaTextChild(private_data, u"private_data", false, 0, group_type == 4 || group_type == 5 ? 0 : 254);

    for (auto it = xactual.begin(); ok && it != xactual.end(); ++it) {
        ActualEvent ev;
        ok = (*it)->getIntAttribute<uint16_t>(ev.service_id, u"service_id", true) &&
             (*it)->getIntAttribute<uint16_t>(ev.event_id, u"event_id", true);
        actual_events.push_back(ev);
    }

    for (auto it = xother.begin(); ok && it != xother.end(); ++it) {
        OtherEvent ev;
        ok = (*it)->getIntAttribute<uint16_t>(ev.original_network_id, u"original_network_id", true) &&
             (*it)->getIntAttribute<uint16_t>(ev.transport_stream_id, u"transport_stream_id", true) &&
             (*it)->getIntAttribute<uint16_t>(ev.service_id, u"service_id", true) &&
             (*it)->getIntAttribute<uint16_t>(ev.event_id, u"event_id", true);
        other_events.push_back(ev);
    }
    return ok;
}
