//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2024, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCCachingPriorityDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"dsmcc_caching_priority_descriptor"
#define MY_CLASS    ts::DSMCCCachingPriorityDescriptor
#define MY_DID      ts::DID_DSMCC_CACHING_PRIORITY
#define MY_TID      ts::TID_DSMCC_UNM
#define MY_STD      ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);

//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::DSMCCCachingPriorityDescriptor::DSMCCCachingPriorityDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::DSMCCCachingPriorityDescriptor::DSMCCCachingPriorityDescriptor(DuckContext& duck, const Descriptor& desc) :
    DSMCCCachingPriorityDescriptor()
{
    deserialize(duck, desc);
}

void ts::DSMCCCachingPriorityDescriptor::clearContent()
{
    priority_value = 0;
    transparency_level = 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DSMCCCachingPriorityDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        const uint8_t priority_value = buf.getUInt8();
        const uint8_t transparency_level = buf.getUInt8();
        disp << margin << UString::Format(u"Priority Value: %n", priority_value) << std::endl;
        disp << margin << "Transparency Level: " << DataName(MY_XML_NAME, u"transparency_level", transparency_level, NamesFlags::HEXA_FIRST) << std::endl;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DSMCCCachingPriorityDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(priority_value);
    buf.putUInt8(transparency_level);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCCachingPriorityDescriptor::deserializePayload(PSIBuffer& buf)
{
    priority_value = buf.getUInt8();
    transparency_level = buf.getUInt8();
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCCachingPriorityDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"priority_value", priority_value, true);
    root->setIntAttribute(u"transparency_level", transparency_level, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DSMCCCachingPriorityDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(priority_value, u"priority_value", true) &&
           element->getIntAttribute(transparency_level, u"transparency_level", true);
}
