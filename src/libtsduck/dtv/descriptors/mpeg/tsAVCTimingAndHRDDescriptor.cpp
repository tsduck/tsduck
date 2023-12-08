//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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
    N_90khz.reset();
    K_90khz.reset();
    num_units_in_tick.reset();
    fixed_frame_rate = false;
    temporal_poc = false;
    picture_to_display_conversion = false;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AVCTimingAndHRDDescriptor::serializePayload(PSIBuffer& buf) const
{
    const bool has_90kHz = N_90khz.has_value() && K_90khz.has_value();
    const bool info_present = num_units_in_tick.has_value();
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
