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

#include "tsDVBJApplicationLocationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"dvb_j_application_location_descriptor"
#define MY_CLASS ts::DVBJApplicationLocationDescriptor
#define MY_DID ts::DID_AIT_DVBJ_APP_LOC
#define MY_TID ts::TID_AIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DVBJApplicationLocationDescriptor::DVBJApplicationLocationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    base_directory(),
    classpath_extension(),
    initial_class()
{
}

ts::DVBJApplicationLocationDescriptor::DVBJApplicationLocationDescriptor(DuckContext& duck, const Descriptor& desc) :
    DVBJApplicationLocationDescriptor()
{
    deserialize(duck, desc);
}

void ts::DVBJApplicationLocationDescriptor::clearContent()
{
    base_directory.clear();
    classpath_extension.clear();
    initial_class.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DVBJApplicationLocationDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->append(duck.encodedWithByteLength(base_directory));
    bbp->append(duck.encodedWithByteLength(classpath_extension));
    bbp->append(duck.encoded(initial_class));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DVBJApplicationLocationDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    base_directory.clear();
    classpath_extension.clear();
    initial_class.clear();

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1;

    if (_is_valid) {
        const size_t len1 = data[0];
        data += 1; size -= 1;
        _is_valid = len1 + 1 <= size;
        if (_is_valid) {
            duck.decode(base_directory, data, len1);
            const size_t len2 = data[len1];
            data += len1 + 1; size -= len1 + 1;
            _is_valid = len2 <= size;
            if (_is_valid) {
                duck.decode(classpath_extension, data, len2);
                duck.decode(initial_class, data + len2, size - len2);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DVBJApplicationLocationDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        size_t len = std::min<size_t>(data[0], size - 1);
        strm << margin << "Base directory: \"" << duck.decoded(data + 1, len) << "\"" << std::endl;
        data += 1 + len; size -= 1 + len;
        if (size >= 1) {
            len = std::min<size_t>(data[0], size - 1);
            strm << margin << "Classpath ext: \"" << duck.decoded(data + 1, len) << "\"" << std::endl
                 << margin << "Initial class: \"" << duck.decoded(data + 1 + len, size - len - 1) << "\"" << std::endl;
            size = 0;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DVBJApplicationLocationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"base_directory", base_directory);
    root->setAttribute(u"classpath_extension", classpath_extension);
    root->setAttribute(u"initial_class", initial_class);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DVBJApplicationLocationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(base_directory, u"base_directory", true) &&
           element->getAttribute(classpath_extension, u"classpath_extension", true) &&
           element->getAttribute(initial_class, u"initial_class", true);
}
