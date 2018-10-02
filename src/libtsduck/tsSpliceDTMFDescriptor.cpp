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

#include "tsSpliceDTMFDescriptor.h"
#include "tsSCTE35.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"splice_DTMF_descriptor"
#define MY_DID ts::DID_SPLICE_DTMF
#define MY_TID ts::TID_SCTE35_SIT

TS_XML_TABSPEC_DESCRIPTOR_FACTORY(ts::SpliceDTMFDescriptor, MY_XML_NAME, MY_TID);
TS_ID_DESCRIPTOR_FACTORY(ts::SpliceDTMFDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));
TS_ID_DESCRIPTOR_DISPLAY(ts::SpliceDTMFDescriptor::DisplayDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SpliceDTMFDescriptor::SpliceDTMFDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    identifier(SPLICE_ID_CUEI),
    preroll(0),
    DTMF()
{
    _is_valid = true;
}

ts::SpliceDTMFDescriptor::SpliceDTMFDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    SpliceDTMFDescriptor()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SpliceDTMFDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    const ByteBlock binDTMF(DTMF.toDVB(0, NPOS, charset));
    if (_is_valid && binDTMF.size() <= DTMF_MAX_SIZE) {
        ByteBlockPtr bbp(serializeStart());
        bbp->appendUInt32(identifier);
        bbp->appendUInt8(preroll);
        bbp->append(uint8_t((DTMF.size() << 5) | 0x1F));
        bbp->append(binDTMF);
        serializeEnd(desc, bbp);
    }
    else {
        desc.invalidate();
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SpliceDTMFDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == _tag && size >= 6;

    if (_is_valid) {
        identifier = GetUInt32(data);
        preroll = GetUInt8(data + 4);
        const size_t len = (GetUInt8(data + 5) >> 5) & 0x07;
        _is_valid = len + 6 == size;
        if (_is_valid) {
            DTMF = UString::FromDVB(data + 6, len, charset);
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SpliceDTMFDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 6) {
        strm << margin << UString::Format(u"Identifier: 0x%X", {GetUInt32(data)});
        display.displayIfASCII(data, 4, u" (\"", u"\")");
        strm << std::endl
             << margin << UString::Format(u"Pre-roll: %d x 1/10 second", {GetUInt8(data + 4)})
             << std::endl;
        size_t len = (GetUInt8(data + 5) >> 5) & 0x07;
        data += 6; size -= 6;

        if (len > size) {
            len = size;
        }
        strm << margin << "DTMF: \"" << UString::FromDVB(data, len, display.dvbCharset()) << "\"" << std::endl;
        data += len; size -= len;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SpliceDTMFDescriptor::buildXML(xml::Element* root) const
{
    root->setIntAttribute(u"identifier", identifier, true);
    root->setIntAttribute(u"preroll", preroll);
    root->setAttribute(u"DTMF", DTMF);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::SpliceDTMFDescriptor::fromXML(const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint32_t>(identifier, u"identifier", false, SPLICE_ID_CUEI) &&
        element->getIntAttribute<uint8_t>(preroll, u"preroll", true) &&
        element->getAttribute(DTMF, u"DTMF", true, u"", 0, DTMF_MAX_SIZE);
}
