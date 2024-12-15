//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMPEG4AudioExtensionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"MPEG4_audio_extension_descriptor"
#define MY_CLASS    ts::MPEG4AudioExtensionDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_MPEG_MPEG4_AUDIO_EXT, ts::Standards::MPEG)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MPEG4AudioExtensionDescriptor::MPEG4AudioExtensionDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::MPEG4AudioExtensionDescriptor::clearContent()
{
    audioProfileLevelIndication.clear();
    audioSpecificConfig.clear();
}

ts::MPEG4AudioExtensionDescriptor::MPEG4AudioExtensionDescriptor(DuckContext& duck, const Descriptor& desc) :
    MPEG4AudioExtensionDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MPEG4AudioExtensionDescriptor::serializePayload(PSIBuffer& buf) const
{
    if (audioProfileLevelIndication.size() > MAX_PROFILES) {
        buf.setUserError();
    }
    else {
        buf.putBit(!audioSpecificConfig.empty());
        buf.putReserved(3);
        buf.putBits(audioProfileLevelIndication.size(), 4);
        buf.putBytes(audioProfileLevelIndication);
        if (!audioSpecificConfig.empty()) {
            buf.putUInt8(uint8_t(audioSpecificConfig.size()));
            buf.putBytes(audioSpecificConfig);
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MPEG4AudioExtensionDescriptor::deserializePayload(PSIBuffer& buf)
{
    const bool ASC_flag = buf.getBool();
    buf.skipReservedBits(3);
    const size_t num_of_loops = buf.getBits<size_t>(4);
    buf.getBytes(audioProfileLevelIndication, num_of_loops);
    if (ASC_flag) {
        const size_t ASC_size = buf.getUInt8();
        buf.getBytes(audioSpecificConfig, ASC_size);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MPEG4AudioExtensionDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(1)) {
        const bool ASC_flag = buf.getBool();
        buf.skipReservedBits(3);
        const size_t num_of_loops = buf.getBits<size_t>(4);
        disp << margin << "Number of audio profile levels: " << num_of_loops << std::endl;
        for (size_t i = 0; i < num_of_loops && buf.canReadBytes(1); ++i) {
            disp << margin << UString::Format(u"Audio profile level #%d: %n", i, buf.getUInt8()) << std::endl;
        }
        if (ASC_flag && buf.canReadBytes(1)) {
            const size_t ASC_size = buf.getUInt8();
            disp.displayPrivateData(u"audioSpecificConfig", buf, ASC_size, margin);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MPEG4AudioExtensionDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (uint8_t lev : audioProfileLevelIndication) {
        root->addElement(u"audioProfileLevelIndication")->setIntAttribute(u"value", lev, true);
    }
    root->addHexaTextChild(u"audioSpecificConfig", audioSpecificConfig, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MPEG4AudioExtensionDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xlevels;
    bool ok = element->getChildren(xlevels, u"audioProfileLevelIndication", 0, MAX_PROFILES) &&
              element->getHexaTextChild(audioSpecificConfig, u"audioSpecificConfig", false, 0, MAX_DESCRIPTOR_SIZE - 2 - xlevels.size());

    for (auto xlev : xlevels) {
        uint8_t lev = 0;
        ok = xlev->getIntAttribute(lev, u"value", true) && ok;
        audioProfileLevelIndication.push_back(lev);
    }
    return ok;
}
