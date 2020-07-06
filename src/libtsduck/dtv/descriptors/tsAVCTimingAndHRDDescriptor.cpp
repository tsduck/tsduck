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

#include "tsAVCTimingAndHRDDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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

void ts::AVCTimingAndHRDDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    const bool has_90kHz = N_90khz.set() && K_90khz.set();
    const bool info_present = num_units_in_tick.set();
    bbp->appendUInt8((hrd_management_valid ? 0x80 : 0x00) | 0x7E | (info_present ? 0x01 : 0x00));
    if (info_present) {
        bbp->appendUInt8((has_90kHz ? 0x80 : 0x00) | 0x7F);
        if (has_90kHz) {
            bbp->appendUInt32(N_90khz.value());
            bbp->appendUInt32(K_90khz.value());
        }
        bbp->appendUInt32(num_units_in_tick.value());
    }
    bbp->appendUInt8((fixed_frame_rate ? 0x80 : 0x00) |
                     (temporal_poc ? 0x40 : 0x00) |
                     (picture_to_display_conversion ? 0x20 : 0x00) |
                     0x1F);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AVCTimingAndHRDDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 2;

    N_90khz.clear();
    K_90khz.clear();
    num_units_in_tick.clear();

    if (_is_valid) {
        hrd_management_valid = (data[0] & 0x80) != 0;
        const bool info_present = (data[0] & 0x01) != 0;
        data++; size--;

        if (info_present) {
            const bool has_90kHz = (data[0] & 0x80) != 0;
            data++; size--;
            _is_valid = (!has_90kHz && size >= 4) || (has_90kHz && size >= 12);
            if (_is_valid) {
                if (has_90kHz) {
                    N_90khz = GetUInt32(data);
                    K_90khz = GetUInt32(data + 4);
                    data += 8; size -= 8;
                }
                num_units_in_tick = GetUInt32(data);
                data += 4; size -= 4;
            }
        }

        _is_valid = _is_valid && size == 1;
        if (_is_valid) {
            fixed_frame_rate = (data[0] & 0x80) != 0;
            temporal_poc = (data[0] & 0x40) != 0;
            picture_to_display_conversion = (data[0] & 0x20) != 0;
            data++; size--;
        }
    }

    _is_valid = _is_valid && size == 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AVCTimingAndHRDDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        strm << margin << "HRD management valid: " << UString::TrueFalse((data[0] & 0x80) != 0) << std::endl;
        bool info_present = (data[0] & 0x01) != 0;
        data++; size--;

        bool ok = size >= 1;
        if (info_present) {
            const bool has_90kHz = (data[0] & 0x80) != 0;
            data++; size--;
            if (has_90kHz) {
                ok = size >= 8;
                if (ok) {
                    strm << margin << UString::Format(u"90 kHz: N = %'d, K = %'d", {GetUInt32(data), GetUInt32(data + 4)}) << std::endl;
                    data += 8; size -= 8;
                }
            }
            ok = ok && size >= 4;
            if (ok) {
                strm << margin << UString::Format(u"Num. units in tick: %'d", {GetUInt32(data)}) << std::endl;
                data += 4; size -= 4;
            }
        }
        if (ok && size >= 1) {
            strm << margin << "Fixed frame rate: " << UString::TrueFalse((data[0] & 0x80) != 0) << std::endl
                 << margin << "Temporal picture order count: " << UString::TrueFalse((data[0] & 0x40) != 0) << std::endl
                 << margin << "Picture to display conversion: " << UString::TrueFalse((data[0] & 0x20) != 0) << std::endl;
            data++; size--;
        }
    }

    display.displayExtraData(data, size, indent);
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
            element->getOptionalIntAttribute<uint32_t>(N_90khz, u"N_90khz") &&
            element->getOptionalIntAttribute<uint32_t>(K_90khz, u"K_90khz") &&
            element->getOptionalIntAttribute<uint32_t>(num_units_in_tick, u"num_units_in_tick") &&
            element->getBoolAttribute(fixed_frame_rate, u"fixed_frame_rate", true) &&
            element->getBoolAttribute(temporal_poc, u"temporal_poc", true) &&
            element->getBoolAttribute(picture_to_display_conversion, u"picture_to_display_conversion", true);
}
