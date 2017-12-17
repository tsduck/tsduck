//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  Representation of a private_data_specifier_descriptor
//
//----------------------------------------------------------------------------

#include "tsPrivateDataSpecifierDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"private_data_specifier_descriptor"
#define MY_DID ts::DID_PRIV_DATA_SPECIF

TS_XML_DESCRIPTOR_FACTORY(ts::PrivateDataSpecifierDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::PrivateDataSpecifierDescriptor, ts::EDID(MY_DID));
TS_ID_DESCRIPTOR_DISPLAY(ts::PrivateDataSpecifierDescriptor::DisplayDescriptor, ts::EDID(MY_DID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::PrivateDataSpecifierDescriptor::PrivateDataSpecifierDescriptor(PDS pds_) :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    pds(pds_)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::PrivateDataSpecifierDescriptor::PrivateDataSpecifierDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    pds(0)
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::PrivateDataSpecifierDescriptor::serialize (Descriptor& desc, const DVBCharset* charset) const
{
    uint8_t data[6];
    data[0] = _tag;
    data[1] = 4;
    PutUInt32 (data + 2, pds);

    Descriptor d (data, sizeof(data));
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::PrivateDataSpecifierDescriptor::deserialize (const Descriptor& desc, const DVBCharset* charset)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() == 4;

    if (_is_valid) {
        pds = GetUInt32 (desc.payload());
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::PrivateDataSpecifierDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 4) {
        uint32_t sp = GetUInt32(data);
        data += 4; size -= 4;
        strm << margin << "Specifier: " << names::PrivateDataSpecifier(sp, names::FIRST) << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// Known PDS names in XML files.
//----------------------------------------------------------------------------

namespace {
    const ts::Enumeration KnownPDS({
        {u"eacem",    ts::PDS_EACEM},
        {u"eutelsat", ts::PDS_EUTELSAT},
    });
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::PrivateDataSpecifierDescriptor::buildXML(xml::Element* root) const
{
    root->setIntEnumAttribute(KnownPDS, u"private_data_specifier", pds);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::PrivateDataSpecifierDescriptor::fromXML(const xml::Element* element)
{
    _is_valid =
        checkXMLName(element) &&
        element->getIntEnumAttribute(pds, KnownPDS, u"private_data_specifier", true);
}
