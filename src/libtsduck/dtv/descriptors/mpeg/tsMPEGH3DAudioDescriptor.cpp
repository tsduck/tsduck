//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#define MY_CLASS    ts::MPEGH3DAudioDescriptor
#define MY_EDID     ts::EDID::ExtensionMPEG(ts::XDID_MPEG_MPH3D_AUDIO)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MPEGH3DAudioDescriptor::MPEGH3DAudioDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::MPEGH3DAudioDescriptor::clearContent()
{
    mpegh_3da_profile_level_indication = 0;
    interactivity_enabled = false;
    reference_channel_layout = 0;
    compatibleSetIndication.clear();
    reserved.clear();
}

ts::MPEGH3DAudioDescriptor::MPEGH3DAudioDescriptor(DuckContext& duck, const Descriptor& desc) :
    MPEGH3DAudioDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization.
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(mpegh_3da_profile_level_indication);
    buf.putBit(interactivity_enabled);
    const bool compatibleProfileSetsPresent = !compatibleSetIndication.empty();
    buf.putBit(!compatibleProfileSetsPresent); // bit=0 means present
    buf.putBits(0xFF, 8);
    buf.putBits(reference_channel_layout, 6);
    if (compatibleProfileSetsPresent) {
        buf.putBits(compatibleSetIndication.size(), 8);
        buf.putBytes(compatibleSetIndication);
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
        buf.getBytes(compatibleSetIndication, numCompatibleSets);
    }
    buf.getBytes(reserved);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(3)) {
        disp << margin << "3D-audio profile level indication: " << DataName(MY_XML_NAME, u"mpegh_3da_profile_level_indication", buf.getUInt8(), NamesFlags::NAME_VALUE) << std::endl;
        disp << margin << UString::Format(u"Interactivity enabled: %s", buf.getBool()) << std::endl;
        const bool compatibleProfileSetsPresent = !buf.getBool(); // bit=0 means present
        buf.skipBits(8);
        disp << margin << "Reference channel layout: " << DataName(MY_XML_NAME, u"reference_channel_layout", buf.getBits<uint8_t>(6), NamesFlags::NAME_VALUE | NamesFlags::DECIMAL) << std::endl;
        if (compatibleProfileSetsPresent) {
            const uint8_t numCompatibleSets = buf.getUInt8();
            for (uint8_t i = 0; buf.canRead() && i < numCompatibleSets; i++)
                disp << margin << "Compatible Set Indication: " << DataName(MY_XML_NAME, u"mpegh_3da_profile_level_indication", buf.getUInt8(), NamesFlags::NAME_VALUE) << std::endl;
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
    root->addHexaTextChild(u"compatibleSetIndication", compatibleSetIndication, true);
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
           element->getHexaTextChild(compatibleSetIndication, u"compatibleSetIndication", false, 0, 251) &&
           element->getHexaTextChild(reserved, u"reserved", false, 0, 251);
}
