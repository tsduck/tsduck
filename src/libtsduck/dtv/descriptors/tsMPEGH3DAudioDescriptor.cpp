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

#include "tsMPEGH3DAudioDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"MPEGH_3D_audio_descriptor"
#define MY_CLASS ts::MPEGH3DAudioDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_MPH3D_AUDIO
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MPEGH3DAudioDescriptor::MPEGH3DAudioDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    mpegh_3da_profile_level_indication(0),
    interactivity_enabled(false),
    reference_channel_layout(0),
    CompatibleSetIndication(),
    reserved()
{
}

void ts::MPEGH3DAudioDescriptor::clearContent()
{
    mpegh_3da_profile_level_indication = 0;
    interactivity_enabled = false;
    reference_channel_layout = 0;
    CompatibleSetIndication.clear();
    reserved.clear();
}

ts::MPEGH3DAudioDescriptor::MPEGH3DAudioDescriptor(DuckContext& duck, const Descriptor& desc) :
    MPEGH3DAudioDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::MPEGH3DAudioDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization.
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(mpegh_3da_profile_level_indication);
    buf.putBit(interactivity_enabled);
    const bool compatibleProfileSetsPresent = !CompatibleSetIndication.empty();
    buf.putBit(!compatibleProfileSetsPresent); // bit=0 means present
    buf.putBits(0xFF, 8);
    buf.putBits(reference_channel_layout, 6);
    if (compatibleProfileSetsPresent) {
        buf.putBits(CompatibleSetIndication.size(), 8);
        buf.putBytes(CompatibleSetIndication);
    }
    buf.putBytes(reserved);
}


//----------------------------------------------------------------------------
// Deserialization.
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioDescriptor::deserializePayload(PSIBuffer& buf)
{
    mpegh_3da_profile_level_indication = buf.getUInt8();
    interactivity_enabled = buf.getBool();
    const bool compatibleProfileSetsPresent = !buf.getBool(); // bit=0 means present
    buf.skipBits(8);
    buf.getBits(reference_channel_layout, 6);
    if (compatibleProfileSetsPresent) {
        const uint8_t numCompatibleSets = buf.getUInt8();
        buf.getBytes(CompatibleSetIndication, numCompatibleSets);
    }
    buf.getBytes(reserved);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        disp << margin << "3D-audio profile level indication: " << DataName(MY_XML_NAME, u"mpegh_3da_profile_level_indication", buf.getUInt8(), NamesFlags::VALUE) << std::endl;
        disp << margin << UString::Format(u"Interactivity enabled: %s", {buf.getBool()}) << std::endl;
        const bool compatibleProfileSetsPresent = !buf.getBool(); // bit=0 means present
        buf.skipBits(8);
        disp << margin << "Reference channel layout: " << DataName(MY_XML_NAME, u"reference_channel_layout", buf.getBits<uint8_t>(6), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
        if (compatibleProfileSetsPresent) {
            const uint8_t numCompatibleSets = buf.getUInt8();
            for (uint8_t i = 0; buf.canRead() && i < numCompatibleSets; i++)
                disp << margin << "Compatible Set Indication: " << DataName(MY_XML_NAME, u"mpegh_3da_profile_level_indication", buf.getUInt8(), NamesFlags::VALUE) << std::endl;
        }
        disp.displayPrivateData(u"Reserved data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization.
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"mpegh_3da_profile_level_indication", mpegh_3da_profile_level_indication, true);
    root->setBoolAttribute(u"interactivity_enabled", interactivity_enabled);
    root->setIntAttribute(u"reference_channel_layout", reference_channel_layout, true);
    root->addHexaTextChild(u"CompatibleSetIndication", CompatibleSetIndication, true);
    root->addHexaTextChild(u"reserved", reserved, true);
}


//----------------------------------------------------------------------------
// XML deserialization.
//----------------------------------------------------------------------------

bool ts::MPEGH3DAudioDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(mpegh_3da_profile_level_indication, u"mpegh_3da_profile_level_indication", true) &&
           element->getBoolAttribute(interactivity_enabled, u"interactivity_enabled", true) &&
           element->getIntAttribute(reference_channel_layout, u"reference_channel_layout", true, 0, 0, 0x3F) &&
           element->getHexaTextChild(CompatibleSetIndication, u"CompatibleSetIndication", false, 0, 251) &&
           element->getHexaTextChild(reserved, u"reserved", false, 0, 251);
}
