//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEmergencyInformationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::EmergencyInformationDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it1 : events) {
        buf.putUInt16(it1.service_id);
        buf.putBit(it1.started);
        buf.putBit(it1.signal_level);
        buf.putBits(0xFF, 6);
        buf.pushWriteSequenceWithLeadingLength(8); // area_code_length
        for (const auto& it2 : it1.area_codes) {
            buf.putBits(it2, 12);
            buf.putBits(0xFF, 4);
        }
        buf.popState(); // update area_code_length
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::EmergencyInformationDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Event ev;
        ev.service_id = buf.getUInt16();
        ev.started = buf.getBool();
        ev.signal_level = buf.getBool();
        buf.skipBits(6);
        buf.pushReadSizeFromLength(8); // area_code_length
        while (buf.canRead()) {
            ev.area_codes.push_back(buf.getBits<uint16_t>(12));
            buf.skipBits(4);
        }
        buf.popState(); // end of  area_code_length
        events.push_back(ev);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::EmergencyInformationDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(4)) {
        disp << margin << UString::Format(u"- Event service id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"  Event is started: %s", {buf.getBool()}) << std::endl;
        disp << margin << UString::Format(u"  Signal level: %d", {buf.getBit()}) << std::endl;
        buf.skipBits(6);
        buf.pushReadSizeFromLength(8); // area_code_length
        while (buf.canRead()) {
            disp << margin << UString::Format(u"  Area code: 0x%03X (%<d)", {buf.getBits<uint16_t>(12)}) << std::endl;
            buf.skipBits(4);
        }
        buf.popState(); // end of  area_code_length
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::EmergencyInformationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it1 : events) {
        xml::Element* e = root->addElement(u"event");
        e->setIntAttribute(u"service_id", it1.service_id, true);
        e->setBoolAttribute(u"started", it1.started);
        e->setIntAttribute(u"signal_level", it1.signal_level);
        for (const auto& it2 : it1.area_codes) {
            e->addElement(u"area")->setIntAttribute(u"code", it2, true);
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
        ok = (*it1)->getIntAttribute(ev.service_id, u"service_id", true) &&
             (*it1)->getBoolAttribute(ev.started, u"started", true) &&
             (*it1)->getIntAttribute(ev.signal_level, u"signal_level", true, 0, 0, 1) &&
             (*it1)->getChildren(xarea, u"area");
        for (auto it2 = xarea.begin(); ok && it2 != xarea.end(); ++it2) {
            uint16_t code = 0;
            ok = (*it2)->getIntAttribute(code, u"code", true, 0, 0, 0x0FFF);
            ev.area_codes.push_back(code);
        }
        events.push_back(ev);
    }
    return ok;
}
