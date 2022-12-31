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

#include "tsAVCTimingAndHRDDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"AVC_timing_and_HRD_descriptor"
#define MY_CLASS ts::AVCTimingAndHRDDescriptor
#define MY_DID ts::DID_AVC_TIMING_HRD
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AVCTimingAndHRDDescriptor::AVCTimingAndHRDDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    hrd_management_valid(false),
    N_90khz(),
    K_90khz(),
    num_units_in_tick(),
    fixed_frame_rate(false),
    temporal_poc(false),
    picture_to_display_conversion(false)
{
}

ts::AVCTimingAndHRDDescriptor::AVCTimingAndHRDDescriptor(DuckContext& duck, const Descriptor& desc) :
    AVCTimingAndHRDDescriptor()
{
    deserialize(duck, desc);
}

void ts::AVCTimingAndHRDDescriptor::clearContent()
{
    hrd_management_valid = false;
    N_90khz.clear();
    K_90khz.clear();
    num_units_in_tick.clear();
    fixed_frame_rate = false;
    temporal_poc = false;
    picture_to_display_conversion = false;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AVCTimingAndHRDDescriptor::serializePayload(PSIBuffer& buf) const
{
    const bool has_90kHz = N_90khz.set() && K_90khz.set();
    const bool info_present = num_units_in_tick.set();
    buf.putBit(hrd_management_valid);
    buf.putBits(0xFF, 6);
    buf.putBit(info_present);
    if (info_present) {
        buf.putBit(has_90kHz);
        buf.putBits(0xFF, 7);
        if (has_90kHz) {
            buf.putUInt32(N_90khz.value());
            buf.putUInt32(K_90khz.value());
        }
        buf.putUInt32(num_units_in_tick.value());
    }
    buf.putBit(fixed_frame_rate);
    buf.putBit(temporal_poc);
    buf.putBit(picture_to_display_conversion);
    buf.putBits(0xFF, 5);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AVCTimingAndHRDDescriptor::deserializePayload(PSIBuffer& buf)
{
    hrd_management_valid = buf.getBool();
    buf.skipBits(6);
    const bool info_present = buf.getBool();
    if (info_present) {
        const bool has_90kHz = buf.getBool();
        buf.skipBits(7);
        if (has_90kHz) {
            N_90khz = buf.getUInt32();
            K_90khz = buf.getUInt32();
        }
        num_units_in_tick = buf.getUInt32();
    }
    fixed_frame_rate = buf.getBool();
    temporal_poc = buf.getBool();
    picture_to_display_conversion = buf.getBool();
    buf.skipBits(5);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AVCTimingAndHRDDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "HRD management valid: " << UString::TrueFalse(buf.getBool()) << std::endl;
        buf.skipBits(6);
        const bool info_present = buf.getBool();

        if (info_present && buf.canReadBytes(1)) {
            const bool has_90kHz = buf.getBool();
            buf.skipBits(7);
            if (has_90kHz && buf.canReadBytes(8)) {
                disp << margin << UString::Format(u"90 kHz: N = %'d", {buf.getUInt32()});
                disp << UString::Format(u", K = %'d", {buf.getUInt32()}) << std::endl;
            }
            if (buf.canReadBytes(4)) {
                disp << margin << UString::Format(u"Num. units in tick: %'d", {buf.getUInt32()}) << std::endl;
            }
        }
        if (buf.canReadBytes(1)) {
            disp << margin << "Fixed frame rate: " << UString::TrueFalse(buf.getBool()) << std::endl;
            disp << margin << "Temporal picture order count: " << UString::TrueFalse(buf.getBool()) << std::endl;
            disp << margin << "Picture to display conversion: " << UString::TrueFalse(buf.getBool()) << std::endl;
            buf.skipBits(5);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AVCTimingAndHRDDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"hrd_management_valid", hrd_management_valid);
    root->setOptionalIntAttribute(u"N_90khz", N_90khz);
    root->setOptionalIntAttribute(u"K_90khz", K_90khz);
    root->setOptionalIntAttribute(u"num_units_in_tick", num_units_in_tick);
    root->setBoolAttribute(u"fixed_frame_rate", fixed_frame_rate);
    root->setBoolAttribute(u"temporal_poc", temporal_poc);
    root->setBoolAttribute(u"picture_to_display_conversion", picture_to_display_conversion);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AVCTimingAndHRDDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return  element->getBoolAttribute(hrd_management_valid, u"hrd_management_valid", true) &&
            element->getOptionalIntAttribute(N_90khz, u"N_90khz") &&
            element->getOptionalIntAttribute(K_90khz, u"K_90khz") &&
            element->getOptionalIntAttribute(num_units_in_tick, u"num_units_in_tick") &&
            element->getBoolAttribute(fixed_frame_rate, u"fixed_frame_rate", true) &&
            element->getBoolAttribute(temporal_poc, u"temporal_poc", true) &&
            element->getBoolAttribute(picture_to_display_conversion, u"picture_to_display_conversion", true);
}
