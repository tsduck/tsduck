//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"splice_time_descriptor"
#define MY_DID ts::DID_SPLICE_TIME
#define MY_TID ts::TID_SCTE35_SIT

TS_XML_TABSPEC_DESCRIPTOR_FACTORY(ts::SpliceTimeDescriptor, MY_XML_NAME, MY_TID);
TS_ID_DESCRIPTOR_FACTORY(ts::SpliceTimeDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));
TS_ID_DESCRIPTOR_DISPLAY(ts::SpliceTimeDescriptor::DisplayDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SpliceTimeDescriptor::SpliceTimeDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    identifier(SPLICE_ID_CUEI),
    TAI_seconds(0),
    TAI_ns(0),
    UTC_offset(0)
{
    _is_valid = true;
}

ts::SpliceTimeDescriptor::SpliceTimeDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    SpliceTimeDescriptor()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SpliceTimeDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
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

void ts::SpliceTimeDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == _tag && size == 16;

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
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 16) {
        const uint64_t tai = GetUInt48(data + 4);
        const uint32_t ns = GetUInt32(data + 10);
        const uint16_t off = GetUInt16(data + 14);
        strm << margin << UString::Format(u"Identifier: 0x%X", {GetUInt32(data)});
        display.displayIfASCII(data, 4, u" (\"", u"\")");
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

void ts::SpliceTimeDescriptor::buildXML(xml::Element* root) const
{
    root->setIntAttribute(u"identifier", identifier, true);
    root->setIntAttribute(u"TAI_seconds", TAI_seconds, false);
    root->setIntAttribute(u"TAI_ns", TAI_ns, false);
    root->setIntAttribute(u"UTC_offset", UTC_offset, false);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::SpliceTimeDescriptor::fromXML(const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint32_t>(identifier, u"identifier", false, SPLICE_ID_CUEI) &&
        element->getIntAttribute<uint64_t>(TAI_seconds, u"TAI_seconds", true, 0, 0, TS_UCONST64(0x0000FFFFFFFFFFFF)) &&
        element->getIntAttribute<uint32_t>(TAI_ns, u"TAI_ns", true) &&
        element->getIntAttribute<uint16_t>(UTC_offset, u"UTC_offset", true);
}
