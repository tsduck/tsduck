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

#include "tsRelatedContentDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"

#define MY_XML_NAME u"related_content_descriptor"
#define MY_CLASS ts::RelatedContentDescriptor
#define MY_DID ts::DID_RELATED_CONTENT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::RelatedContentDescriptor::RelatedContentDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::RelatedContentDescriptor::RelatedContentDescriptor(DuckContext& duck, const Descriptor& desc) :
    RelatedContentDescriptor()
{
    deserialize(duck, desc);
}

void ts::RelatedContentDescriptor::clearContent()
{
}


//----------------------------------------------------------------------------
// This descriptor is always empty.
//----------------------------------------------------------------------------

void ts::RelatedContentDescriptor::serializePayload(PSIBuffer& buf) const
{
}

void ts::RelatedContentDescriptor::deserializePayload(PSIBuffer& buf)
{
}

void ts::RelatedContentDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
}

void ts::RelatedContentDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
}

bool ts::RelatedContentDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return true;
}
