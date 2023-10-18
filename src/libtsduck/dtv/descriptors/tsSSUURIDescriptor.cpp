//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSSUURIDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"SSU_uri_descriptor"
#define MY_CLASS ts::SSUURIDescriptor
#define MY_DID ts::DID_UNT_SSU_URI
#define MY_TID ts::TID_UNT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SSUURIDescriptor::SSUURIDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::SSUURIDescriptor::SSUURIDescriptor(DuckContext& duck, const Descriptor& desc) :
    SSUURIDescriptor()
{
    deserialize(duck, desc);
}

void ts::SSUURIDescriptor::clearContent()
{
    max_holdoff_time = 0;
    min_polling_interval = 0;
    uri.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SSUURIDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(max_holdoff_time);
    buf.putUInt8(min_polling_interval);
    buf.putString(uri);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SSUURIDescriptor::deserializePayload(PSIBuffer& buf)
{
    max_holdoff_time = buf.getUInt8();
    min_polling_interval = buf.getUInt8();
    buf.getString(uri);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SSUURIDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"Max holdoff time: %d minutes", {buf.getUInt8()}) << std::endl;
        disp << margin << UString::Format(u"Min polling interval: %d hours", {buf.getUInt8()}) << std::endl;
        disp << margin << "URI: \"" << buf.getString() << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SSUURIDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"max_holdoff_time", max_holdoff_time, false);
    root->setIntAttribute(u"min_polling_interval", min_polling_interval, false);
    root->setAttribute(u"uri", uri);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SSUURIDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(max_holdoff_time, u"max_holdoff_time", true) &&
           element->getIntAttribute(min_polling_interval, u"min_polling_interval", true) &&
           element->getAttribute(uri, u"uri", true, u"", 0, MAX_DESCRIPTOR_SIZE - 4);
}
