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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    free_format(false),
    ID(0),
    layer(0),
    variable_rate_audio(false)
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
