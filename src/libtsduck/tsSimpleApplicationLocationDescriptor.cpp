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

#include "tsSimpleApplicationLocationDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"simple_application_location_descriptor"
#define MY_DID ts::DID_AIT_APP_LOCATION
#define MY_TID ts::TID_AIT

TS_XML_TABSPEC_DESCRIPTOR_FACTORY(ts::SimpleApplicationLocationDescriptor, MY_XML_NAME, MY_TID);
TS_ID_DESCRIPTOR_FACTORY(ts::SimpleApplicationLocationDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));
TS_ID_DESCRIPTOR_DISPLAY(ts::SimpleApplicationLocationDescriptor::DisplayDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::SimpleApplicationLocationDescriptor::SimpleApplicationLocationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    initial_path()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::SimpleApplicationLocationDescriptor::SimpleApplicationLocationDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    SimpleApplicationLocationDescriptor()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SimpleApplicationLocationDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->append(initial_path.toDVB(0, NPOS, charset));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SimpleApplicationLocationDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    _is_valid = desc.isValid() && desc.tag() == _tag;

    if (_is_valid) {
        initial_path = UString::FromDVB(desc.payload(), desc.payloadSize(), charset);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SimpleApplicationLocationDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    strm << margin << "Initial path: \"" << UString::FromDVB(data, size, display.dvbCharset()) << "\"" << std::endl;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SimpleApplicationLocationDescriptor::buildXML(xml::Element* root) const
{
    root->setAttribute(u"initial_path", initial_path);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::SimpleApplicationLocationDescriptor::fromXML(const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getAttribute(initial_path, u"initial_path", true, UString(), 0, MAX_DESCRIPTOR_SIZE - 2);
}
