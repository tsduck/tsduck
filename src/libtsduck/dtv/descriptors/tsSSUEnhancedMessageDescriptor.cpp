//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsSSUEnhancedMessageDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    descriptor_number(0),
    last_descriptor_number(0),
    ISO_639_language_code(),
    message_index(0),
    text()
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

void ts::SSUEnhancedMessageDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(descriptor_number << 4) | (last_descriptor_number & 0x0F));
    if (!SerializeLanguageCode(*bbp, ISO_639_language_code)) {
        desc.invalidate();
        return;
    }
    bbp->appendUInt8(0xE0 | message_index);
    bbp->append(duck.encoded(text));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SSUEnhancedMessageDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 5;

    if (_is_valid) {
        descriptor_number = (data[0] >> 4) & 0x0F;
        last_descriptor_number = data[0] & 0x0F;
        ISO_639_language_code = DeserializeLanguageCode(data + 1);
        message_index = data[4] & 0x1F;
        duck.decode(text, data + 5, size - 5);
    }
    else {
        text.clear();
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SSUEnhancedMessageDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 5) {
        strm << margin << UString::Format(u"Descriptor number: %d, last: %d", {(data[0] >> 4) & 0x0F, data[0] & 0x0F}) << std::endl
             << margin << "Language: " << DeserializeLanguageCode(data + 1) << std::endl
             << margin << UString::Format(u"Message index: %d", {data[4] & 0x1F}) << std::endl
             << margin << "Text: \"" << duck.decoded(data + 5, size - 5) << "\"" << std::endl;
    }
    else {
        display.displayExtraData(data, size, indent);
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
    return element->getIntAttribute<uint8_t>(descriptor_number, u"descriptor_number", true, 0, 0, 15) &&
           element->getIntAttribute<uint8_t>(last_descriptor_number, u"last_descriptor_number", true, 0, 0, 15) &&
           element->getAttribute(ISO_639_language_code, u"ISO_639_language_code", true, u"", 3, 3) &&
           element->getIntAttribute<uint8_t>(message_index, u"message_index", true, 0, 0, 31) &&
           element->getTextChild(text, u"text");
}
