//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCModuleLinkDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"dsmcc_module_link_descriptor"
#define MY_CLASS    ts::DSMCCModuleLinkDescriptor
#define MY_DID      ts::DID_DSMCC_MODULE_LINK
#define MY_TID      ts::TID_DSMCC_UNM
#define MY_STD      ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);

//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::DSMCCModuleLinkDescriptor::DSMCCModuleLinkDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::DSMCCModuleLinkDescriptor::DSMCCModuleLinkDescriptor(DuckContext& duck, const Descriptor& desc) :
    DSMCCModuleLinkDescriptor()
{
    deserialize(duck, desc);
}

void ts::DSMCCModuleLinkDescriptor::clearContent()
{
    position = 0;
    module_id = 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DSMCCModuleLinkDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        const uint8_t  position = buf.getUInt8();
        const uint16_t module_id = buf.getUInt16();
        disp << margin << "Position: " << DataName(MY_XML_NAME, u"position", position, NamesFlags::HEXA_FIRST) << std::endl;
        disp << margin << "Module Id: " << module_id << std::endl;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DSMCCModuleLinkDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(position);
    buf.putUInt16(module_id);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCModuleLinkDescriptor::deserializePayload(PSIBuffer& buf)
{
    position = buf.getUInt8();
    module_id = buf.getUInt16();
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCModuleLinkDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"position", position, true);
    root->setIntAttribute(u"module_id", module_id, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DSMCCModuleLinkDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(position, u"position", true) &&
           element->getIntAttribute(module_id, u"module_id", true);
}
