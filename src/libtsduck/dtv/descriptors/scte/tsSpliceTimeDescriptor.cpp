//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSpliceTimeDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"splice_time_descriptor"
#define MY_CLASS    ts::SpliceTimeDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_SPLICE_TIME, ts::Standards::SCTE, ts::TID_SCTE35_SIT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SpliceTimeDescriptor::SpliceTimeDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::SpliceTimeDescriptor::clearContent()
{
    identifier = SPLICE_ID_CUEI;
    TAI_seconds = 0;
    TAI_ns = 0;
    UTC_offset = 0;
}

ts::SpliceTimeDescriptor::SpliceTimeDescriptor(DuckContext& duck, const Descriptor& desc) :
    SpliceTimeDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SpliceTimeDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt32(identifier);
    buf.putUInt48(TAI_seconds);
    buf.putUInt32(TAI_ns);
    buf.putUInt16(UTC_offset);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SpliceTimeDescriptor::deserializePayload(PSIBuffer& buf)
{
    identifier = buf.getUInt32();
    TAI_seconds = buf.getUInt48();
    TAI_ns = buf.getUInt32();
    UTC_offset = buf.getUInt16();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SpliceTimeDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(16)) {
        // Sometimes, the identifiers are made of ASCII characters. Try to display them.
        disp.displayIntAndASCII(u"Identifier: 0x%08X", buf, 4, margin);
        const uint64_t tai = buf.getUInt48();
        disp << margin << UString::Format(u"TAI: %'d seconds (%s)", tai, Time::UnixTimeToUTC(uint32_t(tai)).format(Time::DATETIME));
        disp << UString::Format(u" + %'d ns", buf.getUInt32());
        disp << UString::Format(u", UTC offset: %'d", buf.getUInt16()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SpliceTimeDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"identifier", identifier, true);
    root->setIntAttribute(u"TAI_seconds", TAI_seconds, false);
    root->setIntAttribute(u"TAI_ns", TAI_ns, false);
    root->setIntAttribute(u"UTC_offset", UTC_offset, false);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SpliceTimeDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(identifier, u"identifier", false, SPLICE_ID_CUEI) &&
           element->getIntAttribute(TAI_seconds, u"TAI_seconds", true, 0, 0, 0x0000FFFFFFFFFFFF) &&
           element->getIntAttribute(TAI_ns, u"TAI_ns", true) &&
           element->getIntAttribute(UTC_offset, u"UTC_offset", true);
}
