//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsExternalESIdDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"external_ES_ID_descriptor"
#define MY_CLASS ts::ExternalESIdDescriptor
#define MY_DID ts::DID_EXT_ES_ID
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ExternalESIdDescriptor::ExternalESIdDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::ExternalESIdDescriptor::clearContent()
{
    external_ES_ID = 0;
}

ts::ExternalESIdDescriptor::ExternalESIdDescriptor(DuckContext& duck, const Descriptor& desc) :
    ExternalESIdDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ExternalESIdDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(external_ES_ID);
}

void ts::ExternalESIdDescriptor::deserializePayload(PSIBuffer& buf)
{
    external_ES_ID = buf.getUInt16();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ExternalESIdDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"External ES id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ExternalESIdDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"external_ES_ID", external_ES_ID, true);
}

bool ts::ExternalESIdDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(external_ES_ID, u"external_ES_ID", true);
}
