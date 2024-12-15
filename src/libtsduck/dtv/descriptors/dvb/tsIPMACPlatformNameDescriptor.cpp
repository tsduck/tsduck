//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPMACPlatformNameDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"IPMAC_platform_name_descriptor"
#define MY_CLASS    ts::IPMACPlatformNameDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_INT_PF_NAME, ts::Standards::DVB, ts::TID_INT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::IPMACPlatformNameDescriptor::IPMACPlatformNameDescriptor(const UString& lang, const UString& name) :
    AbstractDescriptor(MY_EDID, MY_XML_NAME),
    language_code(lang),
    text(name)
{
}

ts::IPMACPlatformNameDescriptor::IPMACPlatformNameDescriptor(DuckContext& duck, const Descriptor& desc) :
    IPMACPlatformNameDescriptor()
{
    deserialize(duck, desc);
}

void ts::IPMACPlatformNameDescriptor::clearContent()
{
    language_code.clear();
    text.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::IPMACPlatformNameDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putLanguageCode(language_code);
    buf.putString(text);
}

void ts::IPMACPlatformNameDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getLanguageCode(language_code);
    buf.getString(text);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::IPMACPlatformNameDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(3)) {
        disp << margin << "Language: " << buf.getLanguageCode() << std::endl;
        disp << margin << "Platform name: " << buf.getString() << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::IPMACPlatformNameDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"language_code", language_code);
    root->setAttribute(u"text", text);
}

bool ts::IPMACPlatformNameDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(language_code, u"language_code", true, UString(), 3, 3) &&
           element->getAttribute(text, u"text", true, UString(), 0, MAX_DESCRIPTOR_SIZE - 5);
}
