//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHEVCTimingAndHRDDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::HEVCTimingAndHRDDescriptor::clearContent()
{
    hrd_management_valid = false;
    target_schedule_idx.reset();
    N_90khz.reset();
    K_90khz.reset();
    num_units_in_tick.reset();
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
    const bool has_90kHz = N_90khz.has_value() && K_90khz.has_value();
    const bool info_present = num_units_in_tick.has_value();
    buf.putBit(hrd_management_valid);
    buf.putBit(!target_schedule_idx.has_value());
    buf.putBits(target_schedule_idx.value_or(0xFF), 5);
    buf.putBit(info_present);
    if (info_present) {
        buf.putBit(!has_90kHz); // inverted logic, note the '!', see issue #1065
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
    const bool target_schedule_idx_not_present = buf.getBool();
    if (target_schedule_idx_not_present) {
        buf.skipBits(5);
    }
    else {
        buf.getBits(target_schedule_idx, 5);
    }
    const bool info_present = buf.getBool();
    if (info_present) {
        const bool has_90kHz = !buf.getBool();  // inverted logic, see serializePayload()
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

void ts::HEVCTimingAndHRDDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "HRD management valid: " << UString::TrueFalse(buf.getBool()) << std::endl;
        if (buf.getBool()) { // target_schedule_idx_not_present
            buf.skipBits(5);
        }
        else {
            disp << margin << UString::Format(u"Target schedule idx: 0x%x (%<d)", {buf.getBits<uint8_t>(5)}) << std::endl;
        }
        if (buf.getBool()) { // info_present
            const bool has_90kHz = !buf.getBool();  // inverted logic, see serializePayload()
            buf.skipBits(7);
            if (has_90kHz && buf.canReadBytes(8)) {
                disp << margin << UString::Format(u"90 kHz: N = %'d", {buf.getUInt32()});
                disp << UString::Format(u", K = %'d", {buf.getUInt32()}) << std::endl;
            }
            if (buf.canReadBytes(4)) {
                disp << margin << UString::Format(u"Num. units in tick: %'d", {buf.getUInt32()}) << std::endl;
            }
        }
    }
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
