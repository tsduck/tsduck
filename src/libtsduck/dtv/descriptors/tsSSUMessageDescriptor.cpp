//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSSUMessageDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"SSU_message_descriptor"
#define MY_CLASS ts::SSUMessageDescriptor
#define MY_DID ts::DID_UNT_MESSAGE
#define MY_TID ts::TID_UNT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SSUMessageDescriptor::SSUMessageDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::SSUMessageDescriptor::clearContent()
{
    descriptor_number = 0;
    last_descriptor_number = 0;
    ISO_639_language_code.clear();
    text.clear();
}

ts::SSUMessageDescriptor::SSUMessageDescriptor(DuckContext& duck, const Descriptor& desc) :
    SSUMessageDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SSUMessageDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(descriptor_number, 4);
    buf.putBits(last_descriptor_number, 4);
    buf.putLanguageCode(ISO_639_language_code);
    buf.putString(text);
}


void ts::SSUMessageDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(descriptor_number, 4);
    buf.getBits(last_descriptor_number, 4);
    buf.getLanguageCode(ISO_639_language_code);
    buf.getString(text);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SSUMessageDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        disp << margin << UString::Format(u"Descriptor number: %d", {buf.getBits<uint8_t>(4)});
        disp << UString::Format(u", last: %d", {buf.getBits<uint8_t>(4)}) << std::endl;
        disp << margin << "Language: " << buf.getLanguageCode() << std::endl;
        disp << margin << "Text: \"" << buf.getString() << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SSUMessageDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"descriptor_number", descriptor_number);
    root->setIntAttribute(u"last_descriptor_number", last_descriptor_number);
    root->setAttribute(u"ISO_639_language_code", ISO_639_language_code);
    root->addElement(u"text")->addText(text);
}

bool ts::SSUMessageDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(descriptor_number, u"descriptor_number", true, 0, 0, 15) &&
           element->getIntAttribute(last_descriptor_number, u"last_descriptor_number", true, 0, 0, 15) &&
           element->getAttribute(ISO_639_language_code, u"ISO_639_language_code", true, u"", 3, 3) &&
           element->getTextChild(text, u"text");
}
