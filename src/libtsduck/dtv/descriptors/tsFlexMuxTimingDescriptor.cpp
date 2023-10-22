//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFlexMuxTimingDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"flexmux_timing_descriptor"
#define MY_CLASS ts::FlexMuxTimingDescriptor
#define MY_DID ts::DID_FLEX_MUX_TIMING
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::FlexMuxTimingDescriptor::FlexMuxTimingDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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

void ts::FlexMuxTimingDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(FCR_ES_ID);
    buf.putUInt32(FCRResolution);
    buf.putUInt8(FCRLength);
    buf.putUInt8(FmxRateLength);
}

void ts::FlexMuxTimingDescriptor::deserializePayload(PSIBuffer& buf)
{
    FCR_ES_ID = buf.getUInt16();
    FCRResolution = buf.getUInt32();
    FCRLength = buf.getUInt8();
    FmxRateLength = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::FlexMuxTimingDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(8)) {
        disp << margin << UString::Format(u"FCR ES ID: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"FCR resolution: %'d cycles/second", {buf.getUInt32()}) << std::endl;
        disp << margin << UString::Format(u"FCR length: %'d", {buf.getUInt8()}) << std::endl;
        disp << margin << UString::Format(u"FMX rate length: %d", {buf.getUInt8()}) << std::endl;
    }

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

bool ts::FlexMuxTimingDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(FCR_ES_ID, u"FCR_ES_ID", true) &&
           element->getIntAttribute(FCRResolution, u"FCRResolution", true) &&
           element->getIntAttribute(FCRLength, u"FCRLength", true) &&
           element->getIntAttribute(FmxRateLength, u"FmxRateLength", true);
}
