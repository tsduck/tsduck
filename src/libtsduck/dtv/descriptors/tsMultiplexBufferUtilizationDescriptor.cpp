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

#include "tsMultiplexBufferUtilizationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"multiplex_buffer_utilization_descriptor"
#define MY_CLASS ts::MultiplexBufferUtilizationDescriptor
#define MY_DID ts::DID_MUX_BUF_USE
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MultiplexBufferUtilizationDescriptor::MultiplexBufferUtilizationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    LTW_offset_lower_bound(),
    LTW_offset_upper_bound()
{
}

ts::MultiplexBufferUtilizationDescriptor::MultiplexBufferUtilizationDescriptor(DuckContext& duck, const Descriptor& desc) :
    MultiplexBufferUtilizationDescriptor()
{
    deserialize(duck, desc);
}

void ts::MultiplexBufferUtilizationDescriptor::clearContent()
{
    LTW_offset_lower_bound.clear();
    LTW_offset_upper_bound.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MultiplexBufferUtilizationDescriptor::serializePayload(PSIBuffer& buf) const
{
    if (LTW_offset_lower_bound.set() && LTW_offset_upper_bound.set()) {
        buf.putBit(1);
        buf.putBits(LTW_offset_lower_bound.value(), 15);
        buf.putBit(1);
        buf.putBits(LTW_offset_upper_bound.value(), 15);
    }
    else {
        buf.putUInt32(0x7FFFFFFF);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MultiplexBufferUtilizationDescriptor::deserializePayload(PSIBuffer& buf)
{
    if (buf.getBool()) {
        buf.getBits(LTW_offset_lower_bound, 15);
        buf.skipBits(1);
        buf.getBits(LTW_offset_upper_bound, 15);
    }
    else {
        buf.skipBits(31);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MultiplexBufferUtilizationDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(4)) {
        const bool valid = buf.getBool();
        disp << margin << "Bound valid: " << UString::YesNo(valid) << std::endl;
        if (valid) {
            disp << margin << UString::Format(u"LTW offset bounds: lower: 0x%X (%<d)", {buf.getBits<uint16_t>(15)});
            buf.skipBits(1);
            disp << UString::Format(u", upper: 0x%X (%<d)", {buf.getBits<uint16_t>(15)}) << std::endl;
        }
        else {
            buf.skipBits(31);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MultiplexBufferUtilizationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setOptionalIntAttribute(u"LTW_offset_lower_bound", LTW_offset_lower_bound);
    root->setOptionalIntAttribute(u"LTW_offset_upper_bound", LTW_offset_upper_bound);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MultiplexBufferUtilizationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok =
        element->getOptionalIntAttribute(LTW_offset_lower_bound, u"LTW_offset_lower_bound", 0x0000, 0x7FFF) &&
        element->getOptionalIntAttribute(LTW_offset_upper_bound, u"LTW_offset_upper_bound", 0x0000, 0x7FFF);

    if (ok && LTW_offset_lower_bound.set() + LTW_offset_upper_bound.set() == 1) {
        ok = false;
        element->report().error(u"attributes LTW_offset_lower_bound and LTW_offset_upper_bound must be both set or both unset in <%s>, line %d",
                                {element->name(), element->lineNumber()});
    }
    return ok;
}
