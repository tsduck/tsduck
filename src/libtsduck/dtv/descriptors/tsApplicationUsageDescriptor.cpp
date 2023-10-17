//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsApplicationUsageDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"application_usage_descriptor"
#define MY_CLASS ts::ApplicationUsageDescriptor
#define MY_DID ts::DID_AIT_APP_USAGE
#define MY_TID ts::TID_AIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ApplicationUsageDescriptor::ApplicationUsageDescriptor(uint8_t type) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    usage_type(type)
{
}

void ts::ApplicationUsageDescriptor::clearContent()
{
    usage_type = 0;
}

ts::ApplicationUsageDescriptor::ApplicationUsageDescriptor(DuckContext& duck, const Descriptor& desc) :
    ApplicationUsageDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ApplicationUsageDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(usage_type);
}

void ts::ApplicationUsageDescriptor::deserializePayload(PSIBuffer& buf)
{
    usage_type = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ApplicationUsageDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << UString::Format(u"Usage type: %d (0x%<X)", {buf.getUInt8()}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ApplicationUsageDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"usage_type", usage_type, true);
}

bool ts::ApplicationUsageDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(usage_type, u"usage_type", true);
}
