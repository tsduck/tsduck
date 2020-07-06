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

#include "tsSpliceDTMFDescriptor.h"
#include "tsDescriptor.h"
#include "tsSCTE35.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"splice_DTMF_descriptor"
#define MY_CLASS ts::SpliceDTMFDescriptor
#define MY_DID ts::DID_SPLICE_DTMF
#define MY_TID ts::TID_SCTE35_SIT
#define MY_STD ts::Standards::SCTE

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SpliceDTMFDescriptor::SpliceDTMFDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    identifier(SPLICE_ID_CUEI),
    preroll(0),
    DTMF()
{
}

void ts::SpliceDTMFDescriptor::clearContent()
{
    identifier = SPLICE_ID_CUEI;
    preroll = 0;
    DTMF.clear();
}

ts::SpliceDTMFDescriptor::SpliceDTMFDescriptor(DuckContext& duck, const Descriptor& desc) :
    SpliceDTMFDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SpliceDTMFDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    const ByteBlock binDTMF(duck.encoded(DTMF));
    if (_is_valid && binDTMF.size() <= DTMF_MAX_SIZE) {
        ByteBlockPtr bbp(serializeStart());
        bbp->appendUInt32(identifier);
        bbp->appendUInt8(preroll);
        bbp->append(uint8_t((binDTMF.size() << 5) | 0x1F));
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

void ts::SpliceDTMFDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 6;

    if (_is_valid) {
        identifier = GetUInt32(data);
        preroll = GetUInt8(data + 4);
        const size_t len = (GetUInt8(data + 5) >> 5) & 0x07;
        _is_valid = len + 6 == size;
        if (_is_valid) {
            duck.decode(DTMF, data + 6, len);
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SpliceDTMFDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 6) {
        strm << margin << UString::Format(u"Identifier: 0x%X", {GetUInt32(data)});
        duck.displayIfASCII(data, 4, u" (\"", u"\")");
        strm << std::endl
             << margin << UString::Format(u"Pre-roll: %d x 1/10 second", {GetUInt8(data + 4)})
             << std::endl;
        size_t len = (GetUInt8(data + 5) >> 5) & 0x07;
        data += 6; size -= 6;

        if (len > size) {
            len = size;
        }
        strm << margin << "DTMF: \"" << duck.decoded(data, len) << "\"" << std::endl;
        data += len; size -= len;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SpliceDTMFDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"identifier", identifier, true);
    root->setIntAttribute(u"preroll", preroll);
    root->setAttribute(u"DTMF", DTMF);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SpliceDTMFDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute<uint32_t>(identifier, u"identifier", false, SPLICE_ID_CUEI) &&
           element->getIntAttribute<uint8_t>(preroll, u"preroll", true) &&
           element->getAttribute(DTMF, u"DTMF", true, u"", 0, DTMF_MAX_SIZE);
}
