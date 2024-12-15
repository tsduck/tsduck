//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSSUEventNameDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"SSU_event_name_descriptor"
#define MY_CLASS    ts::SSUEventNameDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_UNT_SSU_EVENT_NAME, ts::Standards::DVB, ts::TID_UNT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SSUEventNameDescriptor::SSUEventNameDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::SSUEventNameDescriptor::SSUEventNameDescriptor(DuckContext& duck, const Descriptor& desc) :
    SSUEventNameDescriptor()
{
    deserialize(duck, desc);
}

void ts::SSUEventNameDescriptor::clearContent()
{
    ISO_639_language_code.clear();
    name.clear();
    text.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SSUEventNameDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putLanguageCode(ISO_639_language_code);
    buf.putStringWithByteLength(name);
    buf.putStringWithByteLength(text);
}

void ts::SSUEventNameDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getLanguageCode(ISO_639_language_code);
    buf.getStringWithByteLength(name);
    buf.getStringWithByteLength(text);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SSUEventNameDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(4)) {
        disp << margin << "Language: " << buf.getLanguageCode() << std::endl;
        disp << margin << "Event name: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
        disp << margin << "Event text: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SSUEventNameDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"ISO_639_language_code", ISO_639_language_code);
    root->addElement(u"name")->addText(name);
    root->addElement(u"text")->addText(text);
}

bool ts::SSUEventNameDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(ISO_639_language_code, u"ISO_639_language_code", true, u"", 3, 3) &&
           element->getTextChild(name, u"name") &&
           element->getTextChild(text, u"text");
}
