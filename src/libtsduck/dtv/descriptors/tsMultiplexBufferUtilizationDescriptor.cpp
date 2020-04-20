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

#include "tsMultiplexBufferUtilizationDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"multiplex_buffer_utilization_descriptor"
#define MY_DID ts::DID_MUX_BUF_USE
#define MY_STD ts::STD_MPEG

TS_XML_DESCRIPTOR_FACTORY(ts::MultiplexBufferUtilizationDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::MultiplexBufferUtilizationDescriptor, ts::EDID::Standard(MY_DID));
TS_FACTORY_REGISTER(ts::MultiplexBufferUtilizationDescriptor::DisplayDescriptor, ts::EDID::Standard(MY_DID));


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MultiplexBufferUtilizationDescriptor::MultiplexBufferUtilizationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    LTW_offset_lower_bound(),
    LTW_offset_upper_bound()
{
    _is_valid = true;
}

ts::MultiplexBufferUtilizationDescriptor::MultiplexBufferUtilizationDescriptor(DuckContext& duck, const Descriptor& desc) :
    MultiplexBufferUtilizationDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MultiplexBufferUtilizationDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    if (LTW_offset_lower_bound.set() && LTW_offset_upper_bound.set()) {
        bbp->appendUInt16(0x8000 | LTW_offset_lower_bound.value());
        bbp->appendUInt16(0x8000 | LTW_offset_upper_bound.value());
    }
    else {
        bbp->appendUInt16(0x7FFF);
        bbp->appendUInt16(0xFFFF);
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MultiplexBufferUtilizationDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    LTW_offset_lower_bound.reset();
    LTW_offset_upper_bound.reset();

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == _tag && size == 4;

    if (_is_valid && (data[0] & 0x80) != 0) {
        LTW_offset_lower_bound = GetUInt16(data) & 0x7FFF;
        LTW_offset_upper_bound = GetUInt16(data + 2) & 0x7FFF;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MultiplexBufferUtilizationDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 4) {
        const bool valid = (data[0] & 0x80) != 0;
        const uint16_t low = GetUInt16(data) & 0x7FFF;
        const uint16_t upp = GetUInt16(data + 2) & 0x7FFF;
        strm << margin << "Bound valid: " << UString::YesNo(valid) << std::endl;
        if (valid) {
            strm << margin << UString::Format(u"LTW offset bounds: lower: 0x%X (%d), upper: 0x%X (%d)", {low, low, upp, upp}) << std::endl;
        }
        data += 4; size -= 4;
    }

    display.displayExtraData(data, size, indent);
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

void ts::MultiplexBufferUtilizationDescriptor::fromXML(DuckContext& duck, const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getOptionalIntAttribute<uint16_t>(LTW_offset_lower_bound, u"LTW_offset_lower_bound", 0x0000, 0x7FFF) &&
        element->getOptionalIntAttribute<uint16_t>(LTW_offset_upper_bound, u"LTW_offset_upper_bound", 0x0000, 0x7FFF);

    if (_is_valid && LTW_offset_lower_bound.set() + LTW_offset_upper_bound.set() == 1) {
        _is_valid = false;
        element->report().error(u"attributes LTW_offset_lower_bound and LTW_offset_upper_bound must be both set or both unset in <%s>, line %d",
                                {element->name(), element->lineNumber()});
    }
}
