//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsAACDescriptor.h"
#include "tsComponentDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"AAC_descriptor"
#define MY_CLASS ts::AACDescriptor
#define MY_DID ts::DID_AAC
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AACDescriptor::AACDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    profile_and_level(0),
    SAOC_DE(false),
    AAC_type(),
    additional_info()
{
}

ts::AACDescriptor::AACDescriptor(DuckContext& duck, const Descriptor& desc) :
    AACDescriptor()
{
    deserialize(duck, desc);
}

void ts::AACDescriptor::clearContent()
{
    profile_and_level = 0;
    SAOC_DE = false;
    AAC_type.clear();
    additional_info.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AACDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(profile_and_level);
    if (SAOC_DE || AAC_type.set() || !additional_info.empty()) {
        buf.putBit(AAC_type.set());
        buf.putBit(SAOC_DE);
        buf.putBits(0, 6);
        if (AAC_type.set()) {
            buf.putUInt8(AAC_type.value());
        }
        buf.putBytes(additional_info);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AACDescriptor::deserializePayload(PSIBuffer& buf)
{
    profile_and_level = buf.getUInt8();
    if (buf.canRead()) {
        bool has_AAC_type = buf.getBool();
        SAOC_DE = buf.getBool();
        buf.skipBits(6);
        if (has_AAC_type) {
            AAC_type = buf.getUInt8();
        }
        buf.getBytes(additional_info);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AACDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canRead()) {
        disp << margin << UString::Format(u"Profile and level: 0x%X", {buf.getUInt8()}) << std::endl;
    }

    if (buf.canRead()) {
        bool has_AAC_type = buf.getBool();
        disp << margin << UString::Format(u"SOAC DE flag: %s", {buf.getBool()}) << std::endl;
        buf.skipBits(6);
        if (has_AAC_type && buf.canRead()) {
            disp << margin << "AAC type: " << ComponentDescriptor::ComponentTypeName(disp.duck(), 6, 0, buf.getUInt8(), NamesFlags::HEXA_FIRST, 8) << std::endl;
        }
        disp.displayPrivateData(u"Additional information", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// Get the string representation of an AAC type.
//----------------------------------------------------------------------------

ts::UString ts::AACDescriptor::aacTypeString() const
{
    return isValid() && AAC_type.set() ? aacTypeString(AAC_type.value()) : UString();
}

ts::UString ts::AACDescriptor::aacTypeString(uint8_t type)
{
    DuckContext duck; // only needed by component_descriptor when in Japan.
    return ComponentDescriptor::ComponentTypeName(duck, 6, 0, type, NamesFlags::NAME, 8);
}



//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AACDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"profile_and_level", profile_and_level, true);
    root->setBoolAttribute(u"SAOC_DE", SAOC_DE);
    root->setOptionalIntAttribute(u"AAC_type", AAC_type, true);
    root->addHexaTextChild(u"additional_info", additional_info, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AACDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(profile_and_level, u"profile_and_level", true) &&
           element->getBoolAttribute(SAOC_DE, u"SAOC_DE", false) &&
           element->getOptionalIntAttribute(AAC_type, u"AAC_type") &&
           element->getHexaTextChild(additional_info, u"additional_info", false, 0, MAX_DESCRIPTOR_SIZE - 5);
}
