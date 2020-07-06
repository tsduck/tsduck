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

#include "tsSimpleApplicationBoundaryDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"simple_application_boundary_descriptor"
#define MY_CLASS ts::SimpleApplicationBoundaryDescriptor
#define MY_DID ts::DID_AIT_APP_BOUNDARY
#define MY_TID ts::TID_AIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SimpleApplicationBoundaryDescriptor::SimpleApplicationBoundaryDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    boundary_extension()
{
}

void ts::SimpleApplicationBoundaryDescriptor::clearContent()
{
    boundary_extension.clear();
}

ts::SimpleApplicationBoundaryDescriptor::SimpleApplicationBoundaryDescriptor(DuckContext& duck, const Descriptor& desc) :
    SimpleApplicationBoundaryDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SimpleApplicationBoundaryDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(boundary_extension.size()));
    for (auto it = boundary_extension.begin(); it != boundary_extension.end(); ++it) {
        bbp->append(duck.encodedWithByteLength(*it));
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SimpleApplicationBoundaryDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    boundary_extension.clear();
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1;

    if (_is_valid) {
        const size_t count = data[0];
        data++; size--;
        while (size > 0) {
            boundary_extension.push_back(duck.decodedWithByteLength(data, size));
        }
        _is_valid = count == boundary_extension.size();
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SimpleApplicationBoundaryDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size > 0) {
        strm << margin << UString::Format(u"Number of prefixes: %d", {data[0]}) << std::endl;
        data++; size--;
    }

    while (size > 0) {
        strm << margin << "Boundary extension: \"" << duck.decodedWithByteLength(data, size) << "\"" << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SimpleApplicationBoundaryDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto it = boundary_extension.begin(); it != boundary_extension.end(); ++it) {
        root->addElement(u"prefix")->setAttribute(u"boundary_extension", *it);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SimpleApplicationBoundaryDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"prefix");

    for (size_t i = 0; ok && i < children.size(); ++i) {
        UString s;
        ok = children[i]->getAttribute(s, u"boundary_extension", true);
        boundary_extension.push_back(s);
    }
    return ok;
}
