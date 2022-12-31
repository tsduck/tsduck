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

#include "tsPartialTransportStreamDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"partial_transport_stream_descriptor"
#define MY_CLASS ts::PartialTransportStreamDescriptor
#define MY_DID ts::DID_PARTIAL_TS
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::PartialTransportStreamDescriptor::PartialTransportStreamDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    peak_rate(0),
    minimum_overall_smoothing_rate(UNDEFINED_SMOOTHING_RATE),
    maximum_overall_smoothing_buffer(UNDEFINED_SMOOTHING_BUFFER)
{
}

ts::PartialTransportStreamDescriptor::PartialTransportStreamDescriptor(DuckContext& duck, const Descriptor& desc) :
    PartialTransportStreamDescriptor()
{
    deserialize(duck, desc);
}

void ts::PartialTransportStreamDescriptor::clearContent()
{
    peak_rate = 0;
    minimum_overall_smoothing_rate = UNDEFINED_SMOOTHING_RATE;
    maximum_overall_smoothing_buffer = UNDEFINED_SMOOTHING_BUFFER;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::PartialTransportStreamDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 2);
    buf.putBits(peak_rate, 22);
    buf.putBits(0xFF, 2);
    buf.putBits(minimum_overall_smoothing_rate, 22);
    buf.putBits(0xFF, 2);
    buf.putBits(maximum_overall_smoothing_buffer, 14);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::PartialTransportStreamDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(2);
    buf.getBits(peak_rate, 22);
    buf.skipBits(2);
    buf.getBits(minimum_overall_smoothing_rate, 22);
    buf.skipBits(2);
    buf.getBits(maximum_overall_smoothing_buffer, 14);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::PartialTransportStreamDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(8)) {
        buf.skipBits(2);
        const uint32_t peak = buf.getBits<uint32_t>(22);
        buf.skipBits(2);
        const uint32_t min_rate = buf.getBits<uint32_t>(22);
        buf.skipBits(2);
        const uint16_t max_buffer = buf.getBits<uint16_t>(14);

        disp << margin << UString::Format(u"Peak rate: 0x%X (%<d) x 400 b/s", {peak}) << std::endl;
        disp << margin << "Min smoothing rate: ";
        if (min_rate == UNDEFINED_SMOOTHING_RATE) {
            disp << "undefined";
        }
        else {
            disp << UString::Format(u"0x%X (%<d) x 400 b/s", {min_rate});
        }
        disp << std::endl;
        disp << margin << "Max smoothing buffer: ";
        if (max_buffer == UNDEFINED_SMOOTHING_BUFFER) {
            disp << "undefined";
        }
        else {
            disp << UString::Format(u"0x%X (%<d) bytes", {max_buffer});
        }
        disp << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::PartialTransportStreamDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"peak_rate", peak_rate, true);
    if (minimum_overall_smoothing_rate != UNDEFINED_SMOOTHING_RATE) {
        root->setIntAttribute(u"minimum_overall_smoothing_rate", minimum_overall_smoothing_rate, true);
    }
    if (maximum_overall_smoothing_buffer != UNDEFINED_SMOOTHING_BUFFER) {
        root->setIntAttribute(u"maximum_overall_smoothing_buffer", maximum_overall_smoothing_buffer, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::PartialTransportStreamDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(peak_rate, u"peak_rate", true, 0, 0, 0x003FFFFF) &&
           element->getIntAttribute(minimum_overall_smoothing_rate, u"minimum_overall_smoothing_rate", false, 0x003FFFFF, 0, 0x003FFFFF) &&
           element->getIntAttribute(maximum_overall_smoothing_buffer, u"maximum_overall_smoothing_buffer", false, 0x3FFF, 0, 0x3FFF);
}
