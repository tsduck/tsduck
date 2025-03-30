//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSIParameterDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsMJD.h"

#define MY_XML_NAME u"SI_parameter_descriptor"
#define MY_CLASS    ts::SIParameterDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ISDB_SI_PARAMETER, ts::Standards::ISDB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SIParameterDescriptor::SIParameterDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::SIParameterDescriptor::clearContent()
{
    parameter_version = 0;
    update_time.clear();
    entries.clear();
}

ts::SIParameterDescriptor::SIParameterDescriptor(DuckContext& duck, const Descriptor& desc) :
    SIParameterDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SIParameterDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(parameter_version);
    buf.putMJD(update_time, MJD_DATE);  // 2 bytes: date only
    for (const auto& it : entries) {
        buf.putUInt8(it.table_id);
        buf.putUInt8(uint8_t(it.table_description.size()));
        buf.putBytes(it.table_description);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SIParameterDescriptor::deserializePayload(PSIBuffer& buf)
{
    parameter_version = buf.getUInt8();
    update_time = buf.getMJD(MJD_DATE); // 2 bytes: date only
    while (buf.canRead()) {
        Entry e;
        e.table_id = buf.getUInt8();
        const size_t len = buf.getUInt8();
        buf.getBytes(e.table_description, len);
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SIParameterDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(3)) {
        disp << margin << UString::Format(u"Parameter version: %n", buf.getUInt8()) << std::endl;
        disp << margin << "Update time: " << buf.getMJD(MJD_DATE).format(Time::DATE) << std::endl;
        while (buf.canReadBytes(2)) {
            disp << margin << "- Table id: " << TIDName(disp.duck(), buf.getUInt8(), PID_NULL, CASID_NULL, NamesFlags::HEX_VALUE_NAME) << std::endl;
            disp.displayPrivateData(u"Table description", buf, buf.getUInt8(), margin + u"  ");
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SIParameterDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"parameter_version", parameter_version, true);
    root->setDateAttribute(u"update_time", update_time);
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"table");
        e->setIntAttribute(u"id", it.table_id, true);
        if (!it.table_description.empty()) {
            e->addHexaText(it.table_description);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SIParameterDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xtables;
    bool ok =
        element->getIntAttribute(parameter_version, u"parameter_version", true) &&
        element->getDateAttribute(update_time, u"update_time", true) &&
        element->getChildren(xtables, u"table");

    for (auto it = xtables.begin(); ok && it != xtables.end(); ++it) {
        Entry entry;
        ok = (*it)->getIntAttribute(entry.table_id, u"id", true) &&
             (*it)->getHexaText(entry.table_description, 0, 255);
        entries.push_back(entry);
    }
    return ok;
}
