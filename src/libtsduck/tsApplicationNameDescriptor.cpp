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

#include "tsApplicationNameDescriptor.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"application_name_descriptor"
#define MY_XML_ATTR u"application_name"
#define MY_DID ts::DID_AIT_APP_NAME
#define MY_TID ts::TID_AIT

TS_XML_TABSPEC_DESCRIPTOR_FACTORY(ts::ApplicationNameDescriptor, MY_XML_NAME, MY_TID);
TS_ID_DESCRIPTOR_FACTORY(ts::ApplicationNameDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));
TS_ID_DESCRIPTOR_DISPLAY(ts::ApplicationNameDescriptor::DisplayDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::ApplicationNameDescriptor::ApplicationNameDescriptor() :
    AbstractMultilingualDescriptor(MY_DID, MY_XML_NAME, MY_XML_ATTR)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::ApplicationNameDescriptor::ApplicationNameDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    ApplicationNameDescriptor()
{
    deserialize(desc, charset);
}
