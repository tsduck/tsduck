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
//
//  Representation of a network_name_descriptor
//
//----------------------------------------------------------------------------

#include "tsNetworkNameDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"network_name_descriptor"
#define MY_DID ts::DID_NETWORK_NAME
#define MY_STD ts::STD_DVB

TS_XML_DESCRIPTOR_FACTORY(ts::NetworkNameDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::NetworkNameDescriptor, ts::EDID::Standard(MY_DID));
TS_ID_DESCRIPTOR_DISPLAY(ts::NetworkNameDescriptor::DisplayDescriptor, ts::EDID::Standard(MY_DID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::NetworkNameDescriptor::NetworkNameDescriptor(const UString& name_) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    name(name_)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::NetworkNameDescriptor::NetworkNameDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    name()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::NetworkNameDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->append(name.toDVB(0, NPOS, charset));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::NetworkNameDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    _is_valid = desc.isValid() && desc.tag() == _tag;

    if (_is_valid) {
        name.assign(UString::FromDVB(desc.payload(), desc.payloadSize(), charset));
    }
    else {
        name.clear();
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::NetworkNameDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* payload, size_t size, int indent, TID tid, PDS pds)
{
    display.out() << std::string(indent, ' ')
                  << "Name: \""
                  << UString::FromDVB(payload, size, display.dvbCharset())
                  << "\"" << std::endl;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::NetworkNameDescriptor::buildXML(xml::Element* root) const
{
    root->setAttribute(u"network_name", name);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::NetworkNameDescriptor::fromXML(const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getAttribute(name, u"network_name", true, u"", 0, MAX_DESCRIPTOR_SIZE - 2);
}
