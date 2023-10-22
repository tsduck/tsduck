//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAudioStreamDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"audio_stream_descriptor"
#define MY_CLASS ts::AudioStreamDescriptor
#define MY_DID ts::DID_AUDIO
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AudioStreamDescriptor::AudioStreamDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::AudioStreamDescriptor::AudioStreamDescriptor(DuckContext& duck, const Descriptor& desc) :
    AudioStreamDescriptor()
{
    deserialize(duck, desc);
}

void ts::AudioStreamDescriptor::clearContent()
{
    free_format = false;
    ID = 0;
    layer = 0;
    variable_rate_audio = false;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AudioStreamDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(free_format);
    buf.putBit(ID);
    buf.putBits(layer, 2);
    buf.putBit(variable_rate_audio);
    buf.putBits(0xFF, 3);
}

void ts::AudioStreamDescriptor::deserializePayload(PSIBuffer& buf)
{
    free_format = buf.getBool();
    ID = buf.getBit();
    buf.getBits(layer, 2);
    variable_rate_audio = buf.getBool();
    buf.skipReservedBits(3);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AudioStreamDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "Free format: " << UString::TrueFalse(buf.getBool());
        const uint8_t id = buf.getBit();
        const uint8_t layer = buf.getBits<uint8_t>(2);
        disp << ", variable rate: " << UString::TrueFalse(buf.getBool()) << std::endl;
        disp << margin << UString::Format(u"ID: %d, layer: %d", {id, layer}) << std::endl;
        buf.skipReservedBits(3);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AudioStreamDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"free_format", free_format);
    root->setIntAttribute(u"ID", ID);
    root->setIntAttribute(u"layer", layer);
    root->setBoolAttribute(u"variable_rate_audio", variable_rate_audio);
}

bool ts::AudioStreamDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(free_format, u"free_format", true) &&
           element->getIntAttribute(ID, u"ID", true, 0, 0, 1) &&
           element->getIntAttribute(layer, u"layer", true, 0, 0, 3) &&
           element->getBoolAttribute(variable_rate_audio, u"variable_rate_audio", true);
}
