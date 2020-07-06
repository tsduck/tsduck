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

#include "tsFlexMuxTimingDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"flexmux_timing_descriptor"
#define MY_CLASS ts::FlexMuxTimingDescriptor
#define MY_DID ts::DID_FLEX_MUX_TIMING
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::FlexMuxTimingDescriptor::FlexMuxTimingDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    FCR_ES_ID(0),
    FCRResolution(0),
    FCRLength(0),
    FmxRateLength(0)
{
}

void ts::FlexMuxTimingDescriptor::clearContent()
{
    FCR_ES_ID = 0;
    FCRResolution = 0;
    FCRLength = 0;
    FmxRateLength = 0;
}

ts::FlexMuxTimingDescriptor::FlexMuxTimingDescriptor(DuckContext& duck, const Descriptor& desc) :
    FlexMuxTimingDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::FlexMuxTimingDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt16(FCR_ES_ID);
    bbp->appendUInt32(FCRResolution);
    bbp->appendUInt8(FCRLength);
    bbp->appendUInt8(FmxRateLength);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::FlexMuxTimingDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size == 8;

    if (_is_valid) {
        FCR_ES_ID = GetUInt16(data);
        FCRResolution = GetUInt32(data + 2);
        FCRLength = GetUInt8(data + 6);
        FmxRateLength = GetUInt8(data + 7);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::FlexMuxTimingDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 8) {
        const uint16_t id = GetUInt16(data);
        const uint32_t res = GetUInt32(data + 2);
        const uint8_t len = GetUInt8(data + 6);
        const uint8_t fmx = GetUInt8(data + 7);
        data += 8; size -= 8;
        strm << margin << UString::Format(u"FCR ES ID: 0x%X (%d)", {id, id}) << std::endl
             << margin << UString::Format(u"FCR resolution: %'d cycles/second", {res}) << std::endl
             << margin << UString::Format(u"FCR length: %'d", {len}) << std::endl
             << margin << UString::Format(u"FMX rate length: %d", {fmx}) << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::FlexMuxTimingDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"FCR_ES_ID", FCR_ES_ID, true);
    root->setIntAttribute(u"FCRResolution", FCRResolution);
    root->setIntAttribute(u"FCRLength", FCRLength);
    root->setIntAttribute(u"FmxRateLength", FmxRateLength);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::FlexMuxTimingDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute<uint16_t>(FCR_ES_ID, u"FCR_ES_ID", true) &&
           element->getIntAttribute<uint32_t>(FCRResolution, u"FCRResolution", true) &&
           element->getIntAttribute<uint8_t>(FCRLength, u"FCRLength", true) &&
           element->getIntAttribute<uint8_t>(FmxRateLength, u"FmxRateLength", true);
}
