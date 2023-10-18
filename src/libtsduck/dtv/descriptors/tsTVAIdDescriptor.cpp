//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTVAIdDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"TVA_id_descriptor"
#define MY_CLASS ts::TVAIdDescriptor
#define MY_DID ts::DID_TVA_ID
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TVAIdDescriptor::TVAIdDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::TVAIdDescriptor::TVAIdDescriptor(DuckContext& duck, const Descriptor& desc) :
    TVAIdDescriptor()
{
    deserialize(duck, desc);
}

void ts::TVAIdDescriptor::clearContent()
{
    TVA_ids.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TVAIdDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : TVA_ids) {
        buf.putUInt16(it.TVA_id);
        buf.putBits(0xFF, 5);
        buf.putBits(it.running_status, 3);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TVAIdDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        TVAId ti;
        ti.TVA_id = buf.getUInt16();
        buf.skipBits(5);
        buf.getBits(ti.running_status, 3);
        TVA_ids.push_back(ti);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TVAIdDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(3)) {
        disp << margin << UString::Format(u"TVA id: 0x%X (%<d)", {buf.getUInt16()});
        buf.skipBits(5);
        disp << ", running status: " << DataName(MY_XML_NAME, u"RunningStatus", buf.getBits<uint8_t>(3), NamesFlags::DECIMAL_FIRST) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TVAIdDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : TVA_ids) {
        xml::Element* e = root->addElement(u"TVA");
        e->setIntAttribute(u"id", it.TVA_id, true);
        e->setIntAttribute(u"running_status", it.running_status);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TVAIdDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xtva;
    bool ok = element->getChildren(xtva, u"TVA", 0, MAX_ENTRIES);
    for (auto it = xtva.begin(); ok && it != xtva.end(); ++it) {
        TVAId ti;
        ok = (*it)->getIntAttribute(ti.TVA_id, u"id", true) &&
             (*it)->getIntAttribute(ti.running_status, u"running_status", true, 0, 0, 7);
        TVA_ids.push_back(ti);
    }
    return ok;
}
