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

#include "tsSystemClockDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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

void ts::SystemClockDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8((external_clock_reference ? 0x80 : 0x00) | 0x40 | (clock_accuracy_integer & 0x3F));
    bbp->appendUInt8(uint8_t((clock_accuracy_exponent & 0x07) << 5) | 0x1F);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SystemClockDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size == 2;

    if (_is_valid) {
        external_clock_reference = (data[0] & 0x80) != 0;
        clock_accuracy_integer = data[0] & 0x3F;
        clock_accuracy_exponent = (data[1] >> 5) & 0x07;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SystemClockDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 2) {
        strm << margin << "External clock reference: " << UString::TrueFalse((data[0] & 0x80) != 0) << std::endl
             << margin << UString::Format(u"Clock accuracy integer: %d, exponent: %d", {data[0] & 0x3F, (data[1] >> 5) & 0x07}) << std::endl;
        data += 2; size -= 2;
    }

    display.displayExtraData(data, size, indent);
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
           element->getIntAttribute<uint8_t>(clock_accuracy_integer, u"clock_accuracy_integer", true, 0, 0x00, 0x3F) &&
           element->getIntAttribute<uint8_t>(clock_accuracy_exponent, u"clock_accuracy_exponent", true, 0, 0x00, 0x07);
}
