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

#include "tsSupplementaryAudioDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"supplementary_audio_descriptor"
#define MY_CLASS ts::SupplementaryAudioDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_SUPPL_AUDIO
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SupplementaryAudioDescriptor::SupplementaryAudioDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    mix_type(0),
    editorial_classification(0),
    language_code(),
    private_data()
{
}

ts::SupplementaryAudioDescriptor::SupplementaryAudioDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    mix_type(0),
    editorial_classification(0),
    language_code(),
    private_data()
{
    deserialize(duck, desc);
}

void ts::SupplementaryAudioDescriptor::clearContent()
{
    mix_type = 0;
    editorial_classification = 0;
    language_code.clear();
    private_data.clear();
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::SupplementaryAudioDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SupplementaryAudioDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(mix_type);
    buf.putBits(editorial_classification, 5);
    buf.putBit(1);
    buf.putBit(!language_code.empty());
    if (!language_code.empty()) {
        buf.putLanguageCode(language_code);
    }
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SupplementaryAudioDescriptor::deserializePayload(PSIBuffer& buf)
{
    mix_type = buf.getBit();
    editorial_classification = buf.getBits<uint8_t>(5);
    buf.skipBits(1);
    const bool has_lang = buf.getBit();
    if (has_lang) {
        buf.getLanguageCode(language_code);
    }
    buf.getBytes(private_data);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SupplementaryAudioDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"mix_type", mix_type);
    root->setIntAttribute(u"editorial_classification", editorial_classification, true);
    root->setAttribute(u"language_code", language_code, true);
    root->addHexaTextChild(u"private_data", private_data, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SupplementaryAudioDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute<uint8_t>(mix_type, u"mix_type", true, 0, 0, 1) &&
           element->getIntAttribute<uint8_t>(editorial_classification, u"editorial_classification", true, 0, 0x00, 0x1F) &&
           element->getAttribute(language_code, u"language_code", false, u"", 3, 3) &&
           element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 7);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SupplementaryAudioDescriptor::DisplayDescriptor(TablesDisplay& disp, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    // Important: With extension descriptors, the DisplayDescriptor() function is called
    // with extension payload. Meaning that data points after descriptor_tag_extension.
    // See ts::TablesDisplay::displayDescriptorData()

    const UString margin(indent, ' ');

    if (size >= 1) {
        const uint8_t mix_type = (data[0] >> 7) & 0x01;
        const uint8_t editorial = (data[0] >> 2) & 0x1F;
        const uint8_t lang_present = data[0] & 0x01;
        data++; size--;
        disp << margin << "Mix type: ";
        switch (mix_type) {
            case 0:  disp << "supplementary stream"; break;
            case 1:  disp << "complete and independent stream"; break;
            default: assert(false);
        }
        disp << std::endl;
        disp << margin << "Editorial classification: ";
        switch (editorial) {
            case 0x00: disp << "main audio"; break;
            case 0x01: disp << "audio description for the visually impaired"; break;
            case 0x02: disp << "clean audio for the hearing impaired"; break;
            case 0x03: disp << "spoken subtitles for the visually impaired"; break;
            default:   disp << UString::Format(u"reserved value 0x%X", {editorial}); break;
        }
        disp << std::endl;
        if (lang_present && size >= 3) {
            disp << margin << "Language: " << DeserializeLanguageCode(data) << std::endl;
            data += 3; size -= 3;
        }
        disp.displayPrivateData(u"Private data", data, size, margin);
    }
    else {
        disp.displayExtraData(data, size, margin);
    }
}
