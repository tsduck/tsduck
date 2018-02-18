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
//
//  Representation of an eacem_preferred_name_identifier_descriptor.
//  Private descriptor, must be preceeded by the EACEM/EICTA PDS.
//
//----------------------------------------------------------------------------

#include "tsEacemPreferredNameIdentifierDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"eacem_preferred_name_identifier_descriptor"
#define MY_DID ts::DID_PREF_NAME_ID
#define MY_PDS ts::PDS_EACEM

TS_XML_DESCRIPTOR_FACTORY(ts::EacemPreferredNameIdentifierDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::EacemPreferredNameIdentifierDescriptor, ts::EDID::Private(MY_DID, MY_PDS));
TS_ID_DESCRIPTOR_DISPLAY(ts::EacemPreferredNameIdentifierDescriptor::DisplayDescriptor, ts::EDID::Private(MY_DID, MY_PDS));

// Incorrect use of TPS private data, TPS broadcasters should use EACEM/EICTA PDS instead.
TS_ID_DESCRIPTOR_FACTORY(ts::EacemPreferredNameIdentifierDescriptor, ts::EDID::Private(MY_DID, ts::PDS_TPS));
TS_ID_DESCRIPTOR_DISPLAY(ts::EacemPreferredNameIdentifierDescriptor::DisplayDescriptor, ts::EDID::Private(MY_DID, ts::PDS_TPS));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::EacemPreferredNameIdentifierDescriptor::EacemPreferredNameIdentifierDescriptor(uint8_t id) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_PDS),
    name_id(id)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::EacemPreferredNameIdentifierDescriptor::EacemPreferredNameIdentifierDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_PDS),
    name_id(0)
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::EacemPreferredNameIdentifierDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(name_id);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::EacemPreferredNameIdentifierDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() == 1;

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        name_id = data[0];
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::EacemPreferredNameIdentifierDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        uint8_t id = data[0];
        data += 1; size -= 1;
        strm << margin << "Name identifier: " << int(id) << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::EacemPreferredNameIdentifierDescriptor::buildXML(xml::Element* root) const
{
    root->setIntAttribute(u"name_id", name_id, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::EacemPreferredNameIdentifierDescriptor::fromXML(const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint8_t>(name_id, u"name_id", true, 0, 0x00, 0xFF);
}
