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

#include "tsEmergencyInformationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"emergency_information_descriptor"
#define MY_CLASS ts::EmergencyInformationDescriptor
#define MY_DID ts::DID_ISDB_EMERGENCY_INFO
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::EmergencyInformationDescriptor::EmergencyInformationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    events()
{
}

void ts::EmergencyInformationDescriptor::clearContent()
{
    events.clear();
}

ts::EmergencyInformationDescriptor::EmergencyInformationDescriptor(DuckContext& duck, const Descriptor& desc) :
    EmergencyInformationDescriptor()
{
    deserialize(duck, desc);
}

ts::EmergencyInformationDescriptor::Event::Event() :
    service_id(0),
    started(false),
    signal_level(0),
    area_codes()
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::EmergencyInformationDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    for (auto it1 = events.begin(); it1 != events.end(); ++it1) {
        bbp->appendUInt16(it1->service_id);
        bbp->appendUInt8((it1->started ? 0x80 : 0x00) | (it1->signal_level != 0 ? 0x7F : 0x3F));
        bbp->appendUInt8(uint8_t(2 * it1->area_codes.size()));
        for (auto it2 = it1->area_codes.begin(); it2 != it1->area_codes.end(); ++it2) {
            bbp->appendUInt16(uint16_t(*it2 << 4) | 0x000F);
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::EmergencyInformationDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag();

    events.clear();

    while (_is_valid && size >= 4) {
        Event ev;
        ev.service_id = GetUInt16(data);
        ev.started = (data[2] & 0x80) != 0;
        ev.signal_level = (data[2] >> 6) & 0x01;
        size_t len = data[3];
        data += 4; size -= 4;

        if (len > size || len % 2 != 0) {
            _is_valid = false;
        }
        else {
            while (len >= 2) {
                ev.area_codes.push_back((GetUInt16(data) >> 4) & 0x0FFF);
                data += 2; size -= 2; len -= 2;
            }
            events.push_back(ev);
        }
    }
    _is_valid = _is_valid && size == 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::EmergencyInformationDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    while (size >= 4) {
        strm << margin << UString::Format(u"- Event service id: 0x%X (%d)", {GetUInt16(data), GetUInt16(data)}) << std::endl
             << margin << UString::Format(u"  Event is started: %s", {(data[2] & 0x80) != 0}) << std::endl
             << margin << UString::Format(u"  Signal level: %d", {(data[2] >> 6) & 0x01}) << std::endl;
        size_t len = data[3];
        data += 4; size -= 4;
        len = std::min(len, size);
        while (len >= 2) {
            const uint16_t ac = (GetUInt16(data) >> 4) & 0x0FFF;
            strm << margin << UString::Format(u"  Area code: 0x%03X (%d)", {ac, ac}) << std::endl;
            data += 2; size -= 2; len -= 2;
        }
        if (len > 0) {
            break;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::EmergencyInformationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto it1 = events.begin(); it1 != events.end(); ++it1) {
        xml::Element* e = root->addElement(u"event");
        e->setIntAttribute(u"service_id", it1->service_id, true);
        e->setBoolAttribute(u"started", it1->started);
        e->setIntAttribute(u"signal_level", it1->signal_level);
        for (auto it2 = it1->area_codes.begin(); it2 != it1->area_codes.end(); ++it2) {
            e->addElement(u"area")->setIntAttribute(u"code", *it2, true);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::EmergencyInformationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xevent;
    bool ok = element->getChildren(xevent, u"event");

    for (auto it1 = xevent.begin(); ok && it1 != xevent.end(); ++it1) {
        Event ev;
        xml::ElementVector xarea;
        ok = (*it1)->getIntAttribute<uint16_t>(ev.service_id, u"service_id", true) &&
             (*it1)->getBoolAttribute(ev.started, u"started", true) &&
             (*it1)->getIntAttribute<uint8_t>(ev.signal_level, u"signal_level", true, 0, 0, 1) &&
             (*it1)->getChildren(xarea, u"area");
        for (auto it2 = xarea.begin(); ok && it2 != xarea.end(); ++it2) {
            uint16_t code = 0;
            ok = (*it2)->getIntAttribute<uint16_t>(code, u"code", true, 0, 0, 0x0FFF);
            ev.area_codes.push_back(code);
        }
        events.push_back(ev);
    }
    return ok;
}
