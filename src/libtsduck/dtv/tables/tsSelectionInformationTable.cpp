//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSelectionInformationTable.h"
#include "tsRST.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"selection_information_table"
#define MY_CLASS ts::SelectionInformationTable
#define MY_TID ts::TID_SIT
#define MY_PID ts::PID_SIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SelectionInformationTable::SelectionInformationTable(uint8_t version_, bool is_current_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, is_current_),
    descs(this),
    services(this)
{
}

ts::SelectionInformationTable::SelectionInformationTable(const SelectionInformationTable& other) :
    AbstractLongTable(other),
    descs(this, other.descs),
    services(this, other.services)
{
}

ts::SelectionInformationTable::SelectionInformationTable(DuckContext& duck, const BinaryTable& table) :
    SelectionInformationTable()
{
    deserialize(duck, table);
}

ts::SelectionInformationTable::Service::Service(const AbstractTable* table, uint8_t status) :
    EntryWithDescriptors(table),
    running_status(status)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::SelectionInformationTable::tableIdExtension() const
{
    return 0xFFFF;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::SelectionInformationTable::clearContent()
{
    descs.clear();
    services.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SelectionInformationTable::deserializePayload(PSIBuffer& buf, const Section& section)
{
    buf.getDescriptorListWithLength(descs);
    while (buf.canRead()) {
        Service& srv(services[buf.getUInt16()]);
        buf.skipReservedBits(1);
        buf.getBits(srv.running_status, 3);
        buf.getDescriptorListWithLength(srv.descs);
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SelectionInformationTable::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // A Selection Information Table is not allowed to use more than one section, see ETSI EN 300 468, 7.1.2.

    buf.putPartialDescriptorListWithLength(descs);
    for (auto it = services.begin(); !buf.error() && it != services.end(); ++it) {
        buf.putUInt16(it->first); // service id
        buf.putBit(1);
        buf.putBits(it->second.running_status, 3);
        buf.putPartialDescriptorListWithLength(it->second.descs);
    }
}


//----------------------------------------------------------------------------
// A static method to display a SelectionInformationTable section.
//----------------------------------------------------------------------------

void ts::SelectionInformationTable::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp.displayDescriptorListWithLength(section, buf, margin, u"Global information:");
    while (buf.canReadBytes(4)) {
        disp << margin << UString::Format(u"Service id: %d (0x%<X)", {buf.getUInt16()});
        buf.skipReservedBits(1);
        disp << ", Status: " << RST::RunningStatusNames.name(buf.getBits<uint8_t>(3)) << std::endl;
        disp.displayDescriptorListWithLength(section, buf, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SelectionInformationTable::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    descs.toXML(duck, root);

    for (const auto& it : services) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"service_id", it.first, true);
        e->setEnumAttribute(RST::RunningStatusNames, u"running_status", it.second.running_status);
        it.second.descs.toXML(duck, e);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SelectionInformationTable::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        descs.fromXML(duck, children, element, u"service");

    for (size_t index = 0; ok && index < children.size(); ++index) {
        uint16_t id = 0;
        ok = children[index]->getIntAttribute(id, u"service_id", true) &&
             children[index]->getIntEnumAttribute(services[id].running_status, RST::RunningStatusNames, u"running_status", true);
             services[id].descs.fromXML(duck, children[index]);
    }
    return ok;
}
