//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSupplementaryAudioDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::SupplementaryAudioDescriptor::SupplementaryAudioDescriptor(DuckContext& duck, const Descriptor& desc) :
    SupplementaryAudioDescriptor()
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
    buf.getBits(editorial_classification, 5);
    buf.skipBits(1);
    const bool has_lang = buf.getBool();
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
    return element->getIntAttribute(mix_type, u"mix_type", true, 0, 0, 1) &&
           element->getIntAttribute(editorial_classification, u"editorial_classification", true, 0, 0x00, 0x1F) &&
           element->getAttribute(language_code, u"language_code", false, u"", 3, 3) &&
           element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 7);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SupplementaryAudioDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "Mix type: " << DataName(MY_XML_NAME, u"MixType", buf.getBit()) << std::endl;
        disp << margin << "Editorial classification: " << DataName(MY_XML_NAME, u"Class", buf.getBits<uint8_t>(5)) << std::endl;
        buf.skipBits(1);
        if (buf.getBool() && buf.canReadBytes(3)) {
            disp << margin << "Language: " << buf.getLanguageCode() << std::endl;
        }
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
    }
}
