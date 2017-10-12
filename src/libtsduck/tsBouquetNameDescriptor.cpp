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
//  Representation of a bouquet_name_descriptor
//
//----------------------------------------------------------------------------

#include "tsBouquetNameDescriptor.h"
#include "tsStringUtils.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
TSDUCK_SOURCE;
TS_XML_DESCRIPTOR_FACTORY(ts::BouquetNameDescriptor, "bouquet_name_descriptor");
TS_ID_DESCRIPTOR_FACTORY(ts::BouquetNameDescriptor, ts::EDID(ts::DID_BOUQUET_NAME));
TS_ID_DESCRIPTOR_DISPLAY(ts::BouquetNameDescriptor::DisplayDescriptor, ts::EDID(ts::DID_BOUQUET_NAME));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::BouquetNameDescriptor::BouquetNameDescriptor(const UString& name_) :
    AbstractDescriptor(DID_BOUQUET_NAME, "bouquet_name_descriptor"),
    name(name_)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::BouquetNameDescriptor::BouquetNameDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    AbstractDescriptor(DID_BOUQUET_NAME, "bouquet_name_descriptor"),
    name()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::BouquetNameDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->append(name.toDVB(0, UString::NPOS, charset));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::BouquetNameDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
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

void ts::BouquetNameDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* payload, size_t size, int indent, TID tid, PDS pds)
{
    display.out() << std::string(indent, ' ')
                  << "Name: \""
                  << UString::FromDVB(payload, size, display.dvbCharset())
                  << "\"" << std::endl;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::BouquetNameDescriptor::toXML(XML& xml, XML::Element* parent) const
{
    XML::Element* root = _is_valid ? xml.addElement(parent, _xml_name) : 0;
    xml.setAttribute(root, "bouquet_name", name);
    return root;
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::BouquetNameDescriptor::fromXML(XML& xml, const XML::Element* element)
{
    _is_valid =
        checkXMLName(xml, element) &&
        xml.getAttribute(name, element, "bouquet_name", true, "", 0, MAX_DESCRIPTOR_SIZE - 2);
}
