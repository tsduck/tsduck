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

#include "tsSimpleApplicationBoundaryDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"simple_application_boundary_descriptor"
#define MY_DID ts::DID_AIT_APP_BOUNDARY
#define MY_TID ts::TID_AIT
#define MY_STD ts::STD_DVB

TS_XML_TABSPEC_DESCRIPTOR_FACTORY(ts::SimpleApplicationBoundaryDescriptor, MY_XML_NAME, MY_TID);
TS_ID_DESCRIPTOR_FACTORY(ts::SimpleApplicationBoundaryDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));
TS_ID_DESCRIPTOR_DISPLAY(ts::SimpleApplicationBoundaryDescriptor::DisplayDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SimpleApplicationBoundaryDescriptor::SimpleApplicationBoundaryDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    boundary_extension()
{
    _is_valid = true;
}

ts::SimpleApplicationBoundaryDescriptor::SimpleApplicationBoundaryDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    SimpleApplicationBoundaryDescriptor()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SimpleApplicationBoundaryDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(boundary_extension.size()));
    for (auto it = boundary_extension.begin(); it != boundary_extension.end(); ++it) {
        bbp->append(it->toDVBWithByteLength(0, NPOS, charset));
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SimpleApplicationBoundaryDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    boundary_extension.clear();
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == _tag && size >= 1;

    if (_is_valid) {
        const size_t count = data[0];
        data++; size--;
        while (size > 0) {
            boundary_extension.push_back(UString::FromDVBWithByteLength(data, size, charset));
        }
        _is_valid = count == boundary_extension.size();
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SimpleApplicationBoundaryDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size > 0) {
        strm << margin << UString::Format(u"Number of prefixes: %d", {data[0]}) << std::endl;
        data++; size--;
    }

    while (size > 0) {
        strm << margin << "Boundary extension: \"" << UString::FromDVBWithByteLength(data, size, display.dvbCharset()) << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SimpleApplicationBoundaryDescriptor::buildXML(xml::Element* root) const
{
    for (auto it = boundary_extension.begin(); it != boundary_extension.end(); ++it) {
        root->addElement(u"prefix")->setAttribute(u"boundary_extension", *it);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::SimpleApplicationBoundaryDescriptor::fromXML(const xml::Element* element)
{
    boundary_extension.clear();
    xml::ElementVector children;
    _is_valid = checkXMLName(element) && element->getChildren(children, u"prefix");

    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        UString s;
        _is_valid = children[i]->getAttribute(s, u"boundary_extension", true);
        if (_is_valid) {
            boundary_extension.push_back(s);
        }
    }
}
