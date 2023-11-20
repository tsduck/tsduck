//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFMCDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"FMC_descriptor"
#define MY_CLASS ts::FMCDescriptor
#define MY_DID ts::DID_FMC
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::FMCDescriptor::FMCDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::FMCDescriptor::clearContent()
{
    entries.clear();
}

ts::FMCDescriptor::FMCDescriptor(DuckContext& duck, const Descriptor& desc) :
    FMCDescriptor()
{
    deserialize(duck, desc);
}

ts::FMCDescriptor::Entry::Entry(uint16_t id, uint8_t fmc) :
    ES_ID(id),
    M4MuxChannel(fmc)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::FMCDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : entries) {
        buf.putUInt16(it.ES_ID);
        buf.putUInt8(it.M4MuxChannel);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::FMCDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Entry e;
        e.ES_ID = buf.getUInt16();
        e.M4MuxChannel = buf.getUInt8();
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::FMCDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(3)) {
        disp << margin << UString::Format(u"ES id: 0x%X (%<d)", {buf.getUInt16()});
        disp << UString::Format(u", M4Mux channel: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::FMCDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"stream");
        e->setIntAttribute(u"ES_ID", it.ES_ID, true);
        e->setIntAttribute(u"M4MuxChannel", it.M4MuxChannel, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::FMCDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"stream", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getIntAttribute(entry.ES_ID, u"ES_ID", true) &&
             children[i]->getIntAttribute(entry.M4MuxChannel, u"M4MuxChannel", true);
        entries.push_back(entry);
    }
    return ok;
}
