//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCInfoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"dsmcc_info_descriptor"
#define MY_CLASS    ts::DSMCCInfoDescriptor
#define MY_DID      ts::DID_DSMCC_INFO
#define MY_TID      ts::TID_DSMCC_UNM
#define MY_STD      ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DSMCCInfoDescriptor::DSMCCInfoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::DSMCCInfoDescriptor::DSMCCInfoDescriptor(DuckContext& duck, const Descriptor& desc) :
    DSMCCInfoDescriptor()
{
    deserialize(duck, desc);
}

void ts::DSMCCInfoDescriptor::clearContent()
{
    language_code.clear();
    info.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DSMCCInfoDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putLanguageCode(language_code);
    buf.putString(info);
}

void ts::DSMCCInfoDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getLanguageCode(language_code);
    buf.getString(info);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DSMCCInfoDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        disp << margin << "Language: \"" << buf.getLanguageCode() << "\"" << std::endl;
        disp << margin << "Module or Group info: \"" << buf.getString() << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCInfoDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"language_code", language_code);
    root->setAttribute(u"info", info);
}

bool ts::DSMCCInfoDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(language_code, u"language_code", true, UString(), 3, 3) &&
           element->getAttribute(info, u"info", true, UString(), 0, MAX_DESCRIPTOR_SIZE - 5);
}
