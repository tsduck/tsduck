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

#include "tsSpliceTimeDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"splice_time_descriptor"
#define MY_CLASS ts::SpliceTimeDescriptor
#define MY_DID ts::DID_SPLICE_TIME
#define MY_TID ts::TID_SCTE35_SIT
#define MY_STD ts::Standards::SCTE

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SpliceTimeDescriptor::SpliceTimeDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    identifier(SPLICE_ID_CUEI),
    TAI_seconds(0),
    TAI_ns(0),
    UTC_offset(0)
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

void ts::SpliceTimeDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt32(identifier);
    bbp->appendUInt48(TAI_seconds);
    bbp->appendUInt32(TAI_ns);
    bbp->appendUInt16(UTC_offset);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SpliceTimeDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size == 16;

    if (_is_valid) {
        identifier = GetUInt32(data);
        TAI_seconds = GetUInt48(data + 4);
        TAI_ns = GetUInt32(data + 10);
        UTC_offset = GetUInt16(data + 14);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SpliceTimeDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 16) {
        const uint64_t tai = GetUInt48(data + 4);
        const uint32_t ns = GetUInt32(data + 10);
        const uint16_t off = GetUInt16(data + 14);
        strm << margin << UString::Format(u"Identifier: 0x%X", {GetUInt32(data)});
        duck.displayIfASCII(data, 4, u" (\"", u"\")");
        strm << std::endl
             << margin
             << UString::Format(u"TAI: %'d seconds (%s) + %'d ns, UTC offset: %'d", {tai, Time::UnixTimeToUTC(uint32_t(tai)).format(Time::DATE | Time::TIME), ns, off})
             << std::endl;
        data += 16; size -= 16;
    }

    display.displayExtraData(data, size, indent);
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
    return element->getIntAttribute<uint32_t>(identifier, u"identifier", false, SPLICE_ID_CUEI) &&
           element->getIntAttribute<uint64_t>(TAI_seconds, u"TAI_seconds", true, 0, 0, TS_UCONST64(0x0000FFFFFFFFFFFF)) &&
           element->getIntAttribute<uint32_t>(TAI_ns, u"TAI_ns", true) &&
           element->getIntAttribute<uint16_t>(UTC_offset, u"UTC_offset", true);
}
