//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
#define MY_CLASS    ts::AVCTimingAndHRDDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_MPEG_AVC_TIMING_HRD, ts::Standards::MPEG)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AVCTimingAndHRDDescriptor::AVCTimingAndHRDDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
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
    N.reset();
    K.reset();
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
    const bool is_90kHz = !(N.has_value() && K.has_value()); // time_base is 90kHz unless N and K are specified
    const bool info_present = num_units_in_tick.has_value();
    buf.putBit(hrd_management_valid);
    buf.putBits(0xFF, 6);
    buf.putBit(info_present);
    if (info_present) {
        buf.putBit(is_90kHz);
        buf.putBits(0xFF, 7);
        if (!is_90kHz) {
            buf.putUInt32(N.value());
            buf.putUInt32(K.value());
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
        const bool is_90kHz = buf.getBool();
        buf.skipBits(7);
        if (!is_90kHz) {
            N = buf.getUInt32();
            K = buf.getUInt32();
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

void ts::AVCTimingAndHRDDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(1)) {
        disp << margin << "HRD management valid: " << UString::TrueFalse(buf.getBool()) << std::endl;
        buf.skipBits(6);
        const bool info_present = buf.getBool();

        if (info_present && buf.canReadBytes(1)) {
            const bool is_90kHz = buf.getBool();  // inverted logic, see serializePayload()
            buf.skipBits(7);
            if (is_90kHz) {
                disp << margin << "AVC time base is 90 kHz" << std::endl;
            }
            else if (buf.canReadBytes(8)) {
                disp << margin << UString::Format(u"time_scale: N = %'d", buf.getUInt32());
                disp << UString::Format(u", K = %'d", buf.getUInt32()) << std::endl;
            }
            if (buf.canReadBytes(4)) {
                disp << margin << UString::Format(u"Num. units in tick: %'d", buf.getUInt32()) << std::endl;
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
    root->setOptionalIntAttribute(u"N", N);
    root->setOptionalIntAttribute(u"K", K);
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
    bool ok = element->getBoolAttribute(hrd_management_valid, u"hrd_management_valid", true) &&
              element->getOptionalIntAttribute(N, u"N") &&
              element->getOptionalIntAttribute(K, u"K") &&
              element->getOptionalIntAttribute(num_units_in_tick, u"num_units_in_tick") &&
              element->getBoolAttribute(fixed_frame_rate, u"fixed_frame_rate", true) &&
              element->getBoolAttribute(temporal_poc, u"temporal_poc", true) &&
              element->getBoolAttribute(picture_to_display_conversion, u"picture_to_display_conversion", true);
    if (ok && (N.has_value() + K.has_value() == 1)) {
        element->report().error(u"neither or both of N and K must be specified in <%s>, line %d", element->name(), element->lineNumber());
        ok = false;
    }
    return ok;
}
