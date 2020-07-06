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

#include "tsDTGGuidanceDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"dtg_guidance_descriptor"
#define MY_CLASS ts::DTGGuidanceDescriptor
#define MY_DID ts::DID_OFCOM_GUIDANCE
#define MY_PDS ts::PDS_OFCOM
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DTGGuidanceDescriptor::DTGGuidanceDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS),
    guidance_type(0),
    ISO_639_language_code(),
    text(),
    guidance_mode(false),
    reserved_future_use()
{
}

void ts::DTGGuidanceDescriptor::clearContent()
{
    guidance_type = 0;
    ISO_639_language_code.clear();
    text.clear();
    guidance_mode = false;
    reserved_future_use.clear();
}

ts::DTGGuidanceDescriptor::DTGGuidanceDescriptor(DuckContext& duck, const Descriptor& desc) :
    DTGGuidanceDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DTGGuidanceDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(0xFC | guidance_type);
    switch (guidance_type) {
        case 0x01:
            bbp->appendUInt8(guidance_mode ? 0xFF : 0xFE);
            TS_FALLTHROUGH
        case 0x00:
            if (!SerializeLanguageCode(*bbp, ISO_639_language_code)) {
                desc.invalidate();
                return;
            }
            bbp->append(duck.encoded(text));
            break;
        default:
            bbp->append(reserved_future_use);
            break;
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DTGGuidanceDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1;

    ISO_639_language_code.clear();
    text.clear();
    reserved_future_use.clear();

    if (_is_valid) {
        guidance_type = data[0] & 0x03;
        data++; size--;
        if (guidance_type == 0x00) {
            _is_valid = size >= 3;
        }
        else if (guidance_type == 0x01) {
            _is_valid = size >= 4;
        }
        if (_is_valid) {
            if (guidance_type == 0x01) {
                guidance_mode = (data[0] & 0x01) != 0;
                data++; size--;
            }
            if (guidance_type <= 0x01) {
                deserializeLanguageCode(ISO_639_language_code, data, size);
                duck.decode(text, data, size);
            }
            else {
                reserved_future_use.copy(data, size);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DTGGuidanceDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        const uint8_t type = data[0] & 0x03;
        data++; size--;
        strm << margin << UString::Format(u"Guidance type: %d", {type}) << std::endl;

        if (type == 0 && size >= 3) {
            strm << margin << "Language: \"" << DeserializeLanguageCode(data) << "\"" << std::endl
                 << margin << "Text: \"" << duck.decoded(data + 3, size - 3) << "\"" << std::endl;
            size = 0;
        }
        else if (type == 1 && size >= 4) {
            strm << margin << "Guidance mode: " << UString::TrueFalse(data[0] & 0x01) << std::endl
                 << margin << "Language: \"" << DeserializeLanguageCode(data + 1) << "\"" << std::endl
                 << margin << "Text: \"" << duck.decoded(data + 4, size - 4) << "\"" << std::endl;
            size = 0;
        }
        else if (type >= 2) {
            display.displayPrivateData(u"Reserved", data, size, indent);
            size = 0;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DTGGuidanceDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"guidance_type", guidance_type);
    switch (guidance_type) {
        case 0x01:
            root->setBoolAttribute(u"guidance_mode", guidance_mode);
            TS_FALLTHROUGH
        case 0x00:
            root->setAttribute(u"ISO_639_language_code", ISO_639_language_code);
            root->setAttribute(u"text", text);
            break;
        default:
            root->addHexaTextChild(u"reserved_future_use", reserved_future_use, true);
            break;
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DTGGuidanceDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute<uint8_t>(guidance_type, u"guidance_type", true, 0, 0, 3) &&
           element->getBoolAttribute(guidance_mode, u"guidance_mode", guidance_type == 1) &&
           element->getAttribute(ISO_639_language_code, u"ISO_639_language_code", guidance_type < 2, UString(), 3, 3) &&
           element->getAttribute(text, u"text", guidance_type < 2, UString(), 0, 250) &&
           element->getHexaTextChild(reserved_future_use, u"reserved_future_use", false, 0, 254);
}
