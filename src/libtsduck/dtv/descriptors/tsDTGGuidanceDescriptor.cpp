//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDTGGuidanceDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS)
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

void ts::DTGGuidanceDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 6);
    buf.putBits(guidance_type, 2);
    if (guidance_type == 0x01) {
        buf.putBits(0xFF, 7);
        buf.putBit(guidance_mode);
    }
    if (guidance_type <= 0x01) {
        buf.putLanguageCode(ISO_639_language_code);
        buf.putString(text);
    }
    else {
        buf.putBytes(reserved_future_use);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DTGGuidanceDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(6);
    buf.getBits(guidance_type, 2);
    if (guidance_type == 0x01) {
        buf.skipBits(7);
        guidance_mode = buf.getBool();
    }
    if (guidance_type <= 0x01) {
        buf.getLanguageCode(ISO_639_language_code);
        buf.getString(text);
    }
    else {
        buf.getBytes(reserved_future_use);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DTGGuidanceDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        buf.skipBits(6);
        const uint8_t type = buf.getBits<uint8_t>(2);
        disp << margin << UString::Format(u"Guidance type: %d", {type}) << std::endl;
        if (type == 0x01 && buf.canReadBytes(1)) {
            buf.skipBits(7);
            disp << margin << "Guidance mode: " << UString::TrueFalse(buf.getBool()) << std::endl;
        }
        if (type > 0x01) {
            disp.displayPrivateData(u"Reserved", buf, NPOS, margin);
        }
        else if (buf.canReadBytes(3)) {
            disp << margin << "Language: \"" << buf.getLanguageCode() << "\"" << std::endl;
            disp << margin << "Text: \"" << buf.getString() << "\"" << std::endl;
        }
    }
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
    return element->getIntAttribute(guidance_type, u"guidance_type", true, 0, 0, 3) &&
           element->getBoolAttribute(guidance_mode, u"guidance_mode", guidance_type == 1) &&
           element->getAttribute(ISO_639_language_code, u"ISO_639_language_code", guidance_type < 2, UString(), 3, 3) &&
           element->getAttribute(text, u"text", guidance_type < 2, UString(), 0, 250) &&
           element->getHexaTextChild(reserved_future_use, u"reserved_future_use", false, 0, 254);
}
