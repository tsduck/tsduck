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

#include "tsAudioStreamDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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

void ts::AudioStreamDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8((free_format ? 0x80 : 0x00) |
                     uint8_t((ID & 0x01) << 6) |
                     uint8_t((layer & 0x03) << 4) |
                     (variable_rate_audio ? 0x08 : 0x00) |
                     0x07);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AudioStreamDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size == 1;

    if (_is_valid) {
        free_format = (data[0] & 0x80) != 0;
        ID = (data[0] >> 6) & 0x01;
        layer = (data[0] >> 4) & 0x03;
        variable_rate_audio = (data[0] & 0x08) != 0;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AudioStreamDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        strm << margin << UString::Format(u"Free format: %s, variable rate: %s", {UString::TrueFalse((data[0] & 0x80) != 0), UString::TrueFalse((data[0] & 0x08) != 0)}) << std::endl
             << margin << UString::Format(u"ID: %d, layer: %d", {(data[0] >> 6) & 0x01, (data[0] >> 4) & 0x03}) << std::endl;
        data++; size--;
    }

    display.displayExtraData(data, size, indent);
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


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AudioStreamDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(free_format, u"free_format", true) &&
           element->getIntAttribute<uint8_t>(ID, u"ID", true, 0, 0, 1) &&
           element->getIntAttribute<uint8_t>(layer, u"layer", true, 0, 0, 3) &&
           element->getBoolAttribute(variable_rate_audio, u"variable_rate_audio", true);
}
