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

#include "tsS2SatelliteDeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsVariable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"S2_satellite_delivery_system_descriptor"
#define MY_CLASS ts::S2SatelliteDeliverySystemDescriptor
#define MY_DID ts::DID_S2_SAT_DELIVERY

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::S2SatelliteDeliverySystemDescriptor::S2SatelliteDeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(MY_DID, DS_DVB_S2, MY_XML_NAME),
    scrambling_sequence_selector(false),
    multiple_input_stream_flag(false),
    backwards_compatibility_indicator(false),
    scrambling_sequence_index(0),
    input_stream_identifier(0)
{
}

ts::S2SatelliteDeliverySystemDescriptor::S2SatelliteDeliverySystemDescriptor(DuckContext& duck, const Descriptor& desc) :
    S2SatelliteDeliverySystemDescriptor()
{
    deserialize(duck, desc);
}

void ts::S2SatelliteDeliverySystemDescriptor::clearContent()
{
    scrambling_sequence_selector = false;
    multiple_input_stream_flag = false;
    backwards_compatibility_indicator = false;
    scrambling_sequence_index = 0;
    input_stream_identifier = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::S2SatelliteDeliverySystemDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8((scrambling_sequence_selector ? 0x80 : 0x00) |
                     (multiple_input_stream_flag ? 0x40 : 0x00) |
                     (backwards_compatibility_indicator ? 0x20 : 0x00) |
                     0x1F);
    if (scrambling_sequence_selector) {
        bbp->appendUInt24(0x00FC0000 | scrambling_sequence_index);
    }
    if (multiple_input_stream_flag) {
        bbp->appendUInt8(input_stream_identifier);
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::S2SatelliteDeliverySystemDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    if (!(_is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() >= 1)) {
        return;
    }

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    scrambling_sequence_selector = (data[0] & 0x80) != 0;
    multiple_input_stream_flag = (data[0] & 0x40) != 0;
    backwards_compatibility_indicator = (data[0] & 0x20) != 0;
    data += 1; size -= 1;

    if (scrambling_sequence_selector) {
        _is_valid = size >= 3;
        if (!_is_valid) {
            return;
        }
        scrambling_sequence_index = GetUInt24(data) & 0x0003FFFF;
        data += 3; size -= 3;
    }
    if (multiple_input_stream_flag) {
        _is_valid = size >= 1;
        if (!_is_valid) {
            return;
        }
        input_stream_identifier = GetUInt8(data);
        data += 1; size -= 1;
    }

    _is_valid = size == 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::S2SatelliteDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        const bool scrambling_sequence_selector = (data[0] & 0x80) != 0;
        const bool multiple_input_stream_flag = (data[0] & 0x40) != 0;
        const bool backwards_compatibility_indicator = (data[0] & 0x20) != 0;
        data += 1; size -= 1;

        strm << margin << "Scrambling sequence: " << UString::TrueFalse(scrambling_sequence_selector)
             << ", multiple input stream: " << UString::TrueFalse(multiple_input_stream_flag)
             << ", backwards compatibility: " << UString::TrueFalse(backwards_compatibility_indicator)
             << std::endl;

        if (scrambling_sequence_selector && size >= 3) {
            strm << margin << UString::Format(u"Scrambling sequence index: 0x%05X", {GetUInt24(data) & 0x0003FFFF}) << std::endl;
            data += 3; size -= 3;
        }
        if (multiple_input_stream_flag && size >= 1) {
            strm << margin << UString::Format(u"Input stream identifier: 0x%X", {data[0]}) << std::endl;
            data += 1; size -= 1;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::S2SatelliteDeliverySystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"backwards_compatibility", backwards_compatibility_indicator);
    if (scrambling_sequence_selector) {
        root->setIntAttribute(u"scrambling_sequence_index", scrambling_sequence_index, true);
    }
    if (multiple_input_stream_flag) {
        root->setIntAttribute(u"input_stream_identifier", input_stream_identifier, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::S2SatelliteDeliverySystemDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    Variable<uint32_t> scrambling;
    Variable<uint8_t> stream;

    bool ok =
        element->getBoolAttribute(backwards_compatibility_indicator, u"backwards_compatibility", true) &&
        element->getOptionalIntAttribute<uint32_t>(scrambling, u"scrambling_sequence_index", 0x00000000, 0x0003FFFF) &&
        element->getOptionalIntAttribute<uint8_t>(stream, u"input_stream_identifier");

    if (scrambling.set()) {
        scrambling_sequence_selector = true;
        scrambling_sequence_index = scrambling.value();
    }
    if (stream.set()) {
        multiple_input_stream_flag = true;
        input_stream_identifier = stream.value();
    }
    return ok;
}
