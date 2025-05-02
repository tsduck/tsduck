//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAstraServiceListNameDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"astra_service_list_name_descriptor"
#define MY_CLASS    ts::AstraServiceListNameDescriptor
#define MY_EDID     ts::EDID::PrivateDVB(ts::DID_ASTRA_SERVICE_LIST_NAME, ts::PDS_ASTRA)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AstraServiceListNameDescriptor::AstraServiceListNameDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::AstraServiceListNameDescriptor::clearContent()
{
    language_code.clear();
    name.clear();
}

ts::AstraServiceListNameDescriptor::AstraServiceListNameDescriptor(DuckContext& duck, const Descriptor& desc) :
    AstraServiceListNameDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AstraServiceListNameDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putLanguageCode(language_code);
    buf.putString(name);
}

void ts::AstraServiceListNameDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getLanguageCode(language_code);
    buf.getString(name);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AstraServiceListNameDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(3)) {
        disp << margin << "Language code: \"" << buf.getLanguageCode() << "\"" << std::endl;
        disp << margin << "Service list name: \"" << buf.getString() << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AstraServiceListNameDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"language_code", language_code);
    root->setAttribute(u"name", name);
}

bool ts::AstraServiceListNameDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(language_code, u"language_code", true, UString(), 3, 3) &&
           element->getAttribute(name, u"name");
}
