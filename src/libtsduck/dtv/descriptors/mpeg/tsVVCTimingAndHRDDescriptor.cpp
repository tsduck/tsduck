//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsVVCTimingAndHRDDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"VVC_timing_and_HRD_descriptor"
#define MY_CLASS ts::VVCTimingAndHRDDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_VVC_TIM_HRD
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::VVCTimingAndHRDDescriptor::VVCTimingAndHRDDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::VVCTimingAndHRDDescriptor::clearContent()
{
    hrd_management_valid = false;
    N_90khz.reset();
    K_90khz.reset();
    num_units_in_tick.reset();
}

ts::VVCTimingAndHRDDescriptor::VVCTimingAndHRDDescriptor(DuckContext& duck, const Descriptor& desc) :
    VVCTimingAndHRDDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::VVCTimingAndHRDDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::VVCTimingAndHRDDescriptor::serializePayload(PSIBuffer& buf) const
{
    const bool has_90kHz = N_90khz.has_value() && K_90khz.has_value();
    const bool info_present = num_units_in_tick.has_value();
    buf.putBit(hrd_management_valid);
    buf.putBits(0xFF, 6);
    buf.putBit(info_present);
    if (info_present) {
        buf.putBit(!has_90kHz);  // inverted logic, note the '!', see issue #1065
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

void ts::VVCTimingAndHRDDescriptor::deserializePayload(PSIBuffer& buf)
{
    hrd_management_valid = buf.getBool();
    buf.skipBits(6);
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

void ts::VVCTimingAndHRDDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "HRD management valid: " << UString::TrueFalse(buf.getBool()) << std::endl;
        buf.skipReservedBits(6);
        if (buf.getBool()) { // info_present
            const bool has_90kHz = !buf.getBool();  // inverted logic, see serializePayload()
            buf.skipReservedBits(7);
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

void ts::VVCTimingAndHRDDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"hrd_management_valid", hrd_management_valid);
    root->setOptionalIntAttribute(u"N_90khz", N_90khz);
    root->setOptionalIntAttribute(u"K_90khz", K_90khz);
    root->setOptionalIntAttribute(u"num_units_in_tick", num_units_in_tick);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::VVCTimingAndHRDDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(hrd_management_valid, u"hrd_management_valid", true) &&
           element->getOptionalIntAttribute(N_90khz, u"N_90khz") &&
           element->getOptionalIntAttribute(K_90khz, u"K_90khz") &&
           element->getOptionalIntAttribute(num_units_in_tick, u"num_units_in_tick");
}
