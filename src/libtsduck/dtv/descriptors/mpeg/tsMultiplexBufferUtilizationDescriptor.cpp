//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::MultiplexBufferUtilizationDescriptor::MultiplexBufferUtilizationDescriptor(DuckContext& duck, const Descriptor& desc) :
    MultiplexBufferUtilizationDescriptor()
{
    deserialize(duck, desc);
}

void ts::MultiplexBufferUtilizationDescriptor::clearContent()
{
    LTW_offset_lower_bound.reset();
    LTW_offset_upper_bound.reset();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MultiplexBufferUtilizationDescriptor::serializePayload(PSIBuffer& buf) const
{
    if (LTW_offset_lower_bound.has_value() && LTW_offset_upper_bound.has_value()) {
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

    if (ok && LTW_offset_lower_bound.has_value() + LTW_offset_upper_bound.has_value() == 1) {
        ok = false;
        element->report().error(u"attributes LTW_offset_lower_bound and LTW_offset_upper_bound must be both set or both unset in <%s>, line %d",
                                {element->name(), element->lineNumber()});
    }
    return ok;
}
