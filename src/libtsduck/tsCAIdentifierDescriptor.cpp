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
//  Representation of a CA_identifier_descriptor
//
//----------------------------------------------------------------------------

#include "tsCAIdentifierDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"CA_identifier_descriptor"
#define MY_DID ts::DID_CA_ID
#define MY_STD ts::STD_DVB

TS_XML_DESCRIPTOR_FACTORY(ts::CAIdentifierDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::CAIdentifierDescriptor, ts::EDID::Standard(MY_DID));
TS_ID_DESCRIPTOR_DISPLAY(ts::CAIdentifierDescriptor::DisplayDescriptor, ts::EDID::Standard(MY_DID));


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CAIdentifierDescriptor::CAIdentifierDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    casids()
{
    _is_valid = true;
}

ts::CAIdentifierDescriptor::CAIdentifierDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    CAIdentifierDescriptor()
{
    deserialize(desc, charset);
}

ts::CAIdentifierDescriptor::CAIdentifierDescriptor(int casid, ...) :
    CAIdentifierDescriptor()
{
    _is_valid = true;
    if (casid >= 0) {
        casids.push_back(uint16_t(casid));
        va_list ap;
        va_start(ap, casid);
        while ((casid = va_arg(ap, int)) >= 0) {
            casids.push_back(uint16_t(casid));
        }
        va_end(ap);
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CAIdentifierDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp (new ByteBlock (2));
    CheckNonNull (bbp.pointer());

    for (size_t n = 0; n < casids.size(); ++n) {
        bbp->appendUInt16 (casids[n]);
    }

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d (bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CAIdentifierDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() % 2 == 0;
    casids.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        while (size >= 2) {
            casids.push_back (GetUInt16 (data));
            data += 2;
            size -= 2;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CAIdentifierDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    while (size >= 2) {
        uint16_t cas_id = GetUInt16(data);
        data += 2; size -= 2;
        strm << margin << "CA System Id: " << names::CASId(cas_id, names::FIRST) << std::endl;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CAIdentifierDescriptor::buildXML(xml::Element* root) const
{
    for (size_t i = 0; i < casids.size(); ++i) {
        root->addElement(u"CA_system_id")->setIntAttribute(u"value", casids[i], true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::CAIdentifierDescriptor::fromXML(const xml::Element* element)
{
    casids.clear();
    xml::ElementVector children;
    _is_valid =
        checkXMLName(element) &&
        element->getChildren(children, u"CA_system_id", 0, (MAX_DESCRIPTOR_SIZE - 2) / 2);
    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        uint16_t id = 0;
        _is_valid = children[i]->getIntAttribute<uint16_t>(id, u"value", true, 0, 0x0000, 0xFFFF);
        if (_is_valid) {
            casids.push_back(id);
        }
    }
}
