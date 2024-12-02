//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIODDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"IOD_descriptor"
#define MY_CLASS ts::IODDescriptor
#define MY_DID ts::DID_MPEG_IOD
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::IODDescriptor::IODDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::IODDescriptor::clearContent()
{
    Scope_of_IOD_label = 0;
    IOD_label = 0;
    InitialObjectDescriptor.clear();
}

ts::IODDescriptor::IODDescriptor(DuckContext& duck, const Descriptor& desc) :
    IODDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::IODDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(Scope_of_IOD_label);
    buf.putUInt8(IOD_label);
    buf.putBytes(InitialObjectDescriptor);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::IODDescriptor::deserializePayload(PSIBuffer& buf)
{
    Scope_of_IOD_label = buf.getUInt8();
    IOD_label = buf.getUInt8();
    buf.getBytes(InitialObjectDescriptor);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::IODDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        disp << margin << "Scope of IOD label: " << DataName(MY_XML_NAME, u"scope", buf.getUInt8(), NamesFlags::BOTH_FIRST) << std::endl;
        disp << margin << UString::Format(u"IOD label: %n", buf.getUInt8()) << std::endl;
        disp.displayPrivateData(u"InitialObjectDescriptor", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::IODDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"Scope_of_IOD_label", Scope_of_IOD_label, true);
    root->setIntAttribute(u"IOD_label", IOD_label, true);
    root->addHexaTextChild(u"InitialObjectDescriptor", InitialObjectDescriptor, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::IODDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(Scope_of_IOD_label, u"Scope_of_IOD_label", true) &&
           element->getIntAttribute(IOD_label, u"IOD_label", true) &&
           element->getHexaTextChild(InitialObjectDescriptor, u"InitialObjectDescriptor", false, 0, MAX_DESCRIPTOR_SIZE - 2);
}
