//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCSSUModuleTypeDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"dsmcc_ssu_module_type_descriptor"
#define MY_CLASS    ts::DSMCCSSUModuleTypeDescriptor
#define MY_DID      ts::DID_DSMCC_SSU_MODULE_TYPE
#define MY_TID      ts::TID_DSMCC_UNM
#define MY_STD      ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::DSMCCSSUModuleTypeDescriptor::DSMCCSSUModuleTypeDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::DSMCCSSUModuleTypeDescriptor::DSMCCSSUModuleTypeDescriptor(DuckContext& duck, const Descriptor& desc) :
    DSMCCSSUModuleTypeDescriptor()
{
    deserialize(duck, desc);
}

void ts::DSMCCSSUModuleTypeDescriptor::clearContent()
{
    ssu_module_type = 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DSMCCSSUModuleTypeDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "SSU Module Type: " << DataName(MY_XML_NAME, u"SSU_module_type", buf.getUInt8(), NamesFlags::HEXA_FIRST) << std::endl;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DSMCCSSUModuleTypeDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(ssu_module_type);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCSSUModuleTypeDescriptor::deserializePayload(PSIBuffer& buf)
{
    ssu_module_type = buf.getUInt8();
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCSSUModuleTypeDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"ssu_module_type", ssu_module_type, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DSMCCSSUModuleTypeDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(ssu_module_type, u"ssu_module_type", true);
}
