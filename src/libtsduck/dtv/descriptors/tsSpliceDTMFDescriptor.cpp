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

#include "tsSpliceDTMFDescriptor.h"
#include "tsDescriptor.h"
#include "tsSCTE35.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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

void ts::SpliceDTMFDescriptor::serializePayload(PSIBuffer& buf) const
{
    if (DTMF.size() > DTMF_MAX_SIZE) {
        buf.setUserError();
    }
    else {
        buf.putUInt32(identifier);
        buf.putUInt8(preroll);
        buf.putBits(DTMF.size(), 3);
        buf.putBits(0xFF, 5);
        buf.putUTF8(DTMF);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SpliceDTMFDescriptor::deserializePayload(PSIBuffer& buf)
{
    identifier = buf.getUInt32();
    preroll = buf.getUInt8();
    const size_t len = buf.getBits<size_t>(3);
    buf.skipBits(5);
    buf.getUTF8(DTMF, len);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SpliceDTMFDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(6)) {
        // Sometimes, the identifier is made of ASCII characters. Try to display them.
        disp.displayIntAndASCII(u"Identifier: 0x%08X", buf, 4, margin);
        disp << margin << UString::Format(u"Pre-roll: %d x 1/10 second", {buf.getUInt8()}) << std::endl;
        const size_t len = buf.getBits<size_t>(3);
        buf.skipBits(5);
        disp << margin << "DTMF: \"" << buf.getUTF8(len) << "\"" << std::endl;
    }
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
    return element->getIntAttribute(identifier, u"identifier", false, SPLICE_ID_CUEI) &&
           element->getIntAttribute(preroll, u"preroll", true) &&
           element->getAttribute(DTMF, u"DTMF", true, u"", 0, DTMF_MAX_SIZE);
}
