//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRST.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"RST"
#define MY_CLASS ts::RST
#define MY_TID ts::TID_RST
#define MY_PID ts::PID_RST
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Definition of names for running status values.
//----------------------------------------------------------------------------

const ts::Enumeration ts::RST::RunningStatusNames({
    {u"undefined",   RS_UNDEFINED},
    {u"not-running", RS_NOT_RUNNING},
    {u"starting",    RS_STARTING},
    {u"pausing",     RS_PAUSING},
    {u"running",     RS_RUNNING},
    {u"off-air",     RS_OFF_AIR},
});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::RST::RST() :
    AbstractTable(MY_TID, MY_XML_NAME, MY_STD)
{
}

ts::RST::RST(DuckContext& duck, const BinaryTable& table) :
    RST()
{
    deserialize(duck, table);
}


//----------------------------------------------------------------------------
// Get the maximum size in bytes of the payload of sections of this table.
//----------------------------------------------------------------------------

size_t ts::RST::maxPayloadSize() const
{
    // Although a "private section" in the MPEG sense, the RST section is limited to 1024 bytes in ETSI EN 300 468.
    return MAX_PSI_SHORT_SECTION_PAYLOAD_SIZE;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::RST::clearContent()
{
    events.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::RST::deserializePayload(PSIBuffer& buf, const Section& section)
{
    while (buf.canRead()) {
        Event event;
        event.transport_stream_id = buf.getUInt16();
        event.original_network_id = buf.getUInt16();
        event.service_id = buf.getUInt16();
        event.event_id = buf.getUInt16();
        buf.skipReservedBits(5);
        buf.getBits(event.running_status, 3);
        events.push_back(event);
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::RST::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    for (auto& it : events) {
        buf.putUInt16(it.transport_stream_id);
        buf.putUInt16(it.original_network_id);
        buf.putUInt16(it.service_id);
        buf.putUInt16(it.event_id);
        buf.putBits(0xFF, 5);
        buf.putBits(it.running_status, 3);
    }
}


//----------------------------------------------------------------------------
// A static method to display a RST section.
//----------------------------------------------------------------------------

void ts::RST::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    while (buf.canReadBytes(9)) {
        disp << margin << UString::Format(u"TS: %d (0x%<X)", {buf.getUInt16()});
        disp << UString::Format(u", Orig. Netw.: %d (0x%<X)", {buf.getUInt16()});
        disp << UString::Format(u", Service: %d (0x%<X)", {buf.getUInt16()});
        disp << UString::Format(u", Event: %d (0x%<X)", {buf.getUInt16()});
        buf.skipReservedBits(5);
        disp << ", Status: " << RunningStatusNames.name(buf.getBits<uint8_t>(3)) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::RST::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto& it : events) {
        xml::Element* e = root->addElement(u"event");
        e->setIntAttribute(u"transport_stream_id", it.transport_stream_id, true);
        e->setIntAttribute(u"original_network_id", it.original_network_id, true);
        e->setIntAttribute(u"service_id", it.service_id, true);
        e->setIntAttribute(u"event_id", it.event_id, true);
        e->setEnumAttribute(RunningStatusNames, u"running_status", it.running_status);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::RST::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"event");

    for (size_t index = 0; ok && index < children.size(); ++index) {
        Event event;
        ok = children[index]->getIntAttribute(event.transport_stream_id, u"transport_stream_id", true, 0, 0x0000, 0xFFFF) &&
             children[index]->getIntAttribute(event.original_network_id, u"original_network_id", true, 0, 0x0000, 0xFFFF) &&
             children[index]->getIntAttribute(event.service_id, u"service_id", true, 0, 0x0000, 0xFFFF) &&
             children[index]->getIntAttribute(event.event_id, u"event_id", true, 0, 0x0000, 0xFFFF) &&
             children[index]->getIntEnumAttribute(event.running_status, RunningStatusNames, u"running_status", true);
        if (ok) {
            events.push_back(event);
        }
    }
    return ok;
}
