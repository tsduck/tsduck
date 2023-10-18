//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMessageDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"message_descriptor"
#define MY_CLASS ts::MessageDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_MESSAGE
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::MessageDescriptor::MessageDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::MessageDescriptor::MessageDescriptor(uint8_t id, const UString& lang, const UString& text) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    message_id(id),
    language_code(lang),
    message(text)
{
}

ts::MessageDescriptor::MessageDescriptor(DuckContext& duck, const Descriptor& desc) :
    MessageDescriptor()
{
    deserialize(duck, desc);
}

void ts::MessageDescriptor::clearContent()
{
    message_id = 0;
    language_code.clear();
    message.clear();
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::MessageDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MessageDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(message_id);
    buf.putLanguageCode(language_code);
    buf.putString(message);
}

void ts::MessageDescriptor::deserializePayload(PSIBuffer& buf)
{
    message_id = buf.getUInt8();
    buf.getLanguageCode(language_code);
    buf.getString(message);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MessageDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"message_id", message_id, true);
    root->setAttribute(u"language_code", language_code);
    root->addElement(u"text")->addText(message);
}


bool ts::MessageDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(message_id, u"message_id", true) &&
           element->getAttribute(language_code, u"language_code", true, u"", 3, 3) &&
           element->getTextChild(message, u"text");
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MessageDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        disp << margin << "Message id: " << int(buf.getUInt8());
        disp << ", language: " << buf.getLanguageCode() << std::endl;
        disp << margin << "Message: \"" << buf.getString() << "\"" << std::endl;
    }
}
