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

#include "tsHEVCTimingAndHRDDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"HEVC_timing_and_HRD_descriptor"
#define MY_CLASS ts::HEVCTimingAndHRDDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_HEVC_TIM_HRD
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::HEVCTimingAndHRDDescriptor::HEVCTimingAndHRDDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    hrd_management_valid(false),
    target_schedule_idx(),
    N_90khz(),
    K_90khz(),
    num_units_in_tick()
{
}

void ts::HEVCTimingAndHRDDescriptor::clearContent()
{
    hrd_management_valid = false;
    target_schedule_idx.clear();
    N_90khz.clear();
    K_90khz.clear();
    num_units_in_tick.clear();
}

ts::HEVCTimingAndHRDDescriptor::HEVCTimingAndHRDDescriptor(DuckContext& duck, const Descriptor& desc) :
    HEVCTimingAndHRDDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::HEVCTimingAndHRDDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::HEVCTimingAndHRDDescriptor::serializePayload(PSIBuffer& buf) const
{
    const bool has_90kHz = N_90khz.set() && K_90khz.set();
    const bool info_present = num_units_in_tick.set();
    buf.putBit(hrd_management_valid);
    buf.putBit(!target_schedule_idx.set());
    buf.putBits(target_schedule_idx.value(0xFF), 5);
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
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::HEVCTimingAndHRDDescriptor::deserializePayload(PSIBuffer& buf)
{
    hrd_management_valid = buf.getBool();
    const bool target_schedule_idx_not_present= buf.getBool();
    if (target_schedule_idx_not_present) {
        buf.skipBits(5);
    }
    else {
        buf.getBits(target_schedule_idx, 5);
    }
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
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::HEVCTimingAndHRDDescriptor::DisplayDescriptor(TablesDisplay& disp, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    // Important: With extension descriptors, the DisplayDescriptor() function is called
    // with extension payload. Meaning that data points after descriptor_tag_extension.
    // See ts::TablesDisplay::displayDescriptorData()

    const UString margin(indent, ' ');

    if (size >= 1) {
        disp << margin << "HRD management valid: " << UString::TrueFalse((data[0] & 0x80) != 0) << std::endl;
        bool info_present = (data[0] & 0x01) != 0;
        if ((data[0] & 0x40) == 0) {
            const uint8_t idx = (data[0] >> 1) & 0x1F;
            disp << margin << UString::Format(u"Target schedule idx: 0x%x (%d)", {idx, idx}) << std::endl;
        }
        data++; size--;

        bool ok = size >= 1;
        if (info_present) {
            const bool has_90kHz = (data[0] & 0x80) != 0;
            data++; size--;
            if (has_90kHz) {
                ok = size >= 8;
                if (ok) {
                    disp << margin << UString::Format(u"90 kHz: N = %'d, K = %'d", {GetUInt32(data), GetUInt32(data + 4)}) << std::endl;
                    data += 8; size -= 8;
                }
            }
            ok = ok && size >= 4;
            if (ok) {
                disp << margin << UString::Format(u"Num. units in tick: %'d", {GetUInt32(data)}) << std::endl;
                data += 4; size -= 4;
            }
        }
    }

    disp.displayExtraData(data, size, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::HEVCTimingAndHRDDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"hrd_management_valid", hrd_management_valid);
    root->setOptionalIntAttribute(u"target_schedule_idx", target_schedule_idx);
    root->setOptionalIntAttribute(u"N_90khz", N_90khz);
    root->setOptionalIntAttribute(u"K_90khz", K_90khz);
    root->setOptionalIntAttribute(u"num_units_in_tick", num_units_in_tick);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::HEVCTimingAndHRDDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(hrd_management_valid, u"hrd_management_valid", true) &&
           element->getOptionalIntAttribute(target_schedule_idx, u"target_schedule_idx", 0x00, 0x1F) &&
           element->getOptionalIntAttribute(N_90khz, u"N_90khz") &&
           element->getOptionalIntAttribute(K_90khz, u"K_90khz") &&
           element->getOptionalIntAttribute(num_units_in_tick, u"num_units_in_tick");
}
