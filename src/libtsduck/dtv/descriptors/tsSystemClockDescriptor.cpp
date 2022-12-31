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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    external_clock_reference(false),
    clock_accuracy_integer(0),
    clock_accuracy_exponent(0)
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
