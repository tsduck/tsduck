//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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

#include "tsNPTEndpointDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"NPT_endpoint_descriptor"
#define MY_DID ts::DID_NPT_ENDPOINT
#define MY_STD ts::STD_MPEG

TS_XML_DESCRIPTOR_FACTORY(ts::NPTEndpointDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::NPTEndpointDescriptor, ts::EDID::Standard(MY_DID));
TS_ID_DESCRIPTOR_DISPLAY(ts::NPTEndpointDescriptor::DisplayDescriptor, ts::EDID::Standard(MY_DID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::NPTEndpointDescriptor::NPTEndpointDescriptor(uint64_t start, uint64_t stop) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    start_NPT(start),
    stop_NPT(stop)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::NPTEndpointDescriptor::NPTEndpointDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    NPTEndpointDescriptor()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::NPTEndpointDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt48(TS_UCONST64(0x0000FFFE00000000) | start_NPT);
    bbp->appendUInt64(TS_UCONST64(0xFFFFFFFE00000000) | stop_NPT);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::NPTEndpointDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() == 14;

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        start_NPT = GetUInt48(data) & TS_UCONST64(0x00000001FFFFFFFF);
        stop_NPT = GetUInt64(data + 6) & TS_UCONST64(0x00000001FFFFFFFF);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::NPTEndpointDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 14) {
        const uint64_t start = GetUInt48(data) & TS_UCONST64(0x00000001FFFFFFFF);
        const uint64_t stop = GetUInt64(data + 6) & TS_UCONST64(0x00000001FFFFFFFF);
        data += 14; size -= 14;

        strm << margin << UString::Format(u"Start NPT: 0x%09X (%d)", {start, start}) << std::endl
             << margin << UString::Format(u"Stop NPT:  0x%09X (%d)", {stop, stop}) << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::NPTEndpointDescriptor::buildXML(xml::Element* root) const
{
    root->setIntAttribute(u"start_NPT", start_NPT, true);
    root->setIntAttribute(u"stop_NPT", stop_NPT, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::NPTEndpointDescriptor::fromXML(const xml::Element* element, const DVBCharset* charset)
{
    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint64_t>(start_NPT, u"start_NPT", true, 0, 0, TS_UCONST64(0x00000001FFFFFFFF)) &&
        element->getIntAttribute<uint64_t>(stop_NPT, u"stop_NPT", true, 0, 0, TS_UCONST64(0x00000001FFFFFFFF));
}
