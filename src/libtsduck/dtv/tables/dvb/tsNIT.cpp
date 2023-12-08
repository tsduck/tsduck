//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNIT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"NIT"
#define MY_CLASS ts::NIT
#define MY_PID ts::PID_NIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {ts::TID_NIT_ACT, ts::TID_NIT_OTH}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors and assignment.
//----------------------------------------------------------------------------

ts::NIT::NIT(bool is_actual, uint8_t vers, bool cur, uint16_t id) :
    AbstractTransportListTable(uint8_t(is_actual ? TID_NIT_ACT : TID_NIT_OTH), MY_XML_NAME, MY_STD, id, vers, cur),
    network_id(_tid_ext)
{
}

ts::NIT::NIT(DuckContext& duck, const BinaryTable& table) :
    AbstractTransportListTable(duck, TID_NIT_ACT, MY_XML_NAME, MY_STD, table),  // TID updated by deserialize()
    network_id(_tid_ext)
{
}

ts::NIT::NIT(const NIT& other) :
    AbstractTransportListTable(other),
    network_id(_tid_ext)
{
}

ts::NIT& ts::NIT::operator=(const NIT& other)
{
    if (&other != this) {
        // Assign super class but leave uint16_t& network_id unchanged.
        AbstractTransportListTable::operator=(other);
    }
    return *this;
}


//----------------------------------------------------------------------------
// This method checks if a table id is valid for this object.
//----------------------------------------------------------------------------

bool ts::NIT::isValidTableId(TID tid) const
{
    return tid == TID_NIT_ACT || tid == TID_NIT_OTH;
}


//----------------------------------------------------------------------------
// Get the maximum size in bytes of the payload of sections of this table.
//----------------------------------------------------------------------------

size_t ts::NIT::maxPayloadSize() const
{
    // Although a "private section" in the MPEG sense, the NIT section is limited to 1024 bytes in ETSI EN 300 468.
    return MAX_PSI_LONG_SECTION_PAYLOAD_SIZE;
}


//----------------------------------------------------------------------------
// A static method to display a NIT section.
//----------------------------------------------------------------------------

void ts::NIT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    // Display bouquet information
    disp << margin << UString::Format(u"Network Id: %d (0x%<X)", {section.tableIdExtension()}) << std::endl;
    disp.displayDescriptorListWithLength(section, buf, margin, u"Network information:");

    // Transport stream loop
    buf.skipReservedBits(4);
    buf.pushReadSizeFromLength(12); // transport_stream_loop_length
    while (buf.canReadBytes(6)) {
        const uint16_t tsid = buf.getUInt16();
        const uint16_t nwid = buf.getUInt16();
        disp << margin << UString::Format(u"Transport Stream Id: %d (0x%<X), Original Network Id: %d (0x%<X)", {tsid, nwid}) << std::endl;
        disp.displayDescriptorListWithLength(section, buf, margin);
    }
    buf.popState(); // transport_stream_loop_length
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::NIT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"network_id", network_id, true);
    root->setBoolAttribute(u"actual", isActual());
    descs.toXML(duck, root);

    for (auto& tp : transports) {
        xml::Element* e = root->addElement(u"transport_stream");
        e->setIntAttribute(u"transport_stream_id", tp.first.transport_stream_id, true);
        e->setIntAttribute(u"original_network_id", tp.first.original_network_id, true);
        if (tp.second.preferred_section >= 0) {
            e->setIntAttribute(u"preferred_section", tp.second.preferred_section, false);
        }
        tp.second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::NIT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool actual = true;
    bool ok =
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute(network_id, u"network_id", true, 0, 0x0000, 0xFFFF) &&
        element->getBoolAttribute(actual, u"actual", false, true) &&
        descs.fromXML(duck, children, element, u"transport_stream");

    setActual(actual);

    for (size_t index = 0; ok && index < children.size(); ++index) {
        TransportStreamId ts;
        ok = children[index]->getIntAttribute(ts.transport_stream_id, u"transport_stream_id", true, 0, 0x0000, 0xFFFF) &&
             children[index]->getIntAttribute(ts.original_network_id, u"original_network_id", true, 0, 0x0000, 0xFFFF) &&
             transports[ts].descs.fromXML(duck, children[index]);
        if (ok && children[index]->hasAttribute(u"preferred_section")) {
            ok = children[index]->getIntAttribute(transports[ts].preferred_section, u"preferred_section", true, 0, 0, 255);
        }
        else {
            transports[ts].preferred_section = -1;
        }
    }
    return ok;
}
