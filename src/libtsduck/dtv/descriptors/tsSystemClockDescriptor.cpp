//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSystemClockDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"system_clock_descriptor"
#define MY_CLASS ts::SystemClockDescriptor
#define MY_DID ts::DID_SYS_CLOCK
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SystemClockDescriptor::SystemClockDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::SystemClockDescriptor::SystemClockDescriptor(DuckContext& duck, const Descriptor& desc) :
    SystemClockDescriptor()
{
    deserialize(duck, desc);
}

void ts::SystemClockDescriptor::clearContent()
{
    external_clock_reference = false;
    clock_accuracy_integer = 0;
    clock_accuracy_exponent = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SystemClockDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(external_clock_reference);
    buf.putBit(1);
    buf.putBits(clock_accuracy_integer, 6);
    buf.putBits(clock_accuracy_exponent, 3);
    buf.putBits(0xFF, 5);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SystemClockDescriptor::deserializePayload(PSIBuffer& buf)
{
    external_clock_reference = buf.getBool();
    buf.skipBits(1);
    buf.getBits(clock_accuracy_integer, 6);
    buf.getBits(clock_accuracy_exponent, 3);
    buf.skipBits(5);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SystemClockDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"External clock reference: %s", {buf.getBool()}) << std::endl;
        buf.skipBits(1);
        disp << margin << UString::Format(u"Clock accuracy integer: %d", {buf.getBits<uint8_t>(6)});
        disp << UString::Format(u", exponent: %d", {buf.getBits<uint8_t>(3)}) << std::endl;
        buf.skipBits(5);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SystemClockDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"external_clock_reference", external_clock_reference);
    root->setIntAttribute(u"clock_accuracy_integer", clock_accuracy_integer);
    root->setIntAttribute(u"clock_accuracy_exponent", clock_accuracy_exponent);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SystemClockDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getBoolAttribute(external_clock_reference, u"external_clock_reference", true) &&
           element->getIntAttribute(clock_accuracy_integer, u"clock_accuracy_integer", true, 0, 0x00, 0x3F) &&
           element->getIntAttribute(clock_accuracy_exponent, u"clock_accuracy_exponent", true, 0, 0x00, 0x07);
}
