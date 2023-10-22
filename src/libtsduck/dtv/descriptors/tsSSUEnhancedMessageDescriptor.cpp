//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSSUEnhancedMessageDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"SSU_enhanced_message_descriptor"
#define MY_CLASS ts::SSUEnhancedMessageDescriptor
#define MY_DID ts::DID_UNT_ENHANCED_MSG
#define MY_TID ts::TID_UNT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SSUEnhancedMessageDescriptor::SSUEnhancedMessageDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::SSUEnhancedMessageDescriptor::clearContent()
{
    descriptor_number = 0;
    last_descriptor_number = 0;
    ISO_639_language_code.clear();
    message_index = 0;
    text.clear();
}

ts::SSUEnhancedMessageDescriptor::SSUEnhancedMessageDescriptor(DuckContext& duck, const Descriptor& desc) :
    SSUEnhancedMessageDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SSUEnhancedMessageDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(descriptor_number, 4);
    buf.putBits(last_descriptor_number, 4);
    buf.putLanguageCode(ISO_639_language_code);
    buf.putBits(0xFF, 3);
    buf.putBits(message_index, 5);
    buf.putString(text);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SSUEnhancedMessageDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(descriptor_number, 4);
    buf.getBits(last_descriptor_number, 4);
    buf.getLanguageCode(ISO_639_language_code);
    buf.skipBits(3);
    buf.getBits(message_index, 5);
    buf.getString(text);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SSUEnhancedMessageDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(5)) {
        disp << margin << UString::Format(u"Descriptor number: %d", {buf.getBits<uint8_t>(4)});
        disp << UString::Format(u", last: %d", {buf.getBits<uint8_t>(4)}) << std::endl;
        disp << margin << "Language: " << buf.getLanguageCode() << std::endl;
        buf.skipBits(3);
        disp << margin << UString::Format(u"Message index: %d", {buf.getBits<uint8_t>(5)}) << std::endl;
        disp << margin << "Text: \"" << buf.getString() << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SSUEnhancedMessageDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"descriptor_number", descriptor_number);
    root->setIntAttribute(u"last_descriptor_number", last_descriptor_number);
    root->setAttribute(u"ISO_639_language_code", ISO_639_language_code);
    root->setIntAttribute(u"message_index", message_index);
    root->addElement(u"text")->addText(text);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SSUEnhancedMessageDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(descriptor_number, u"descriptor_number", true, 0, 0, 15) &&
           element->getIntAttribute(last_descriptor_number, u"last_descriptor_number", true, 0, 0, 15) &&
           element->getAttribute(ISO_639_language_code, u"ISO_639_language_code", true, u"", 3, 3) &&
           element->getIntAttribute(message_index, u"message_index", true, 0, 0, 31) &&
           element->getTextChild(text, u"text");
}
