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

#include "tsTargetIPAddressDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"target_IP_address_descriptor"
#define MY_DID ts::DID_INT_IP_ADDR
#define MY_STD ts::STD_DVB

TS_XML_TABSPEC_DESCRIPTOR_FACTORY(ts::TargetIPAddressDescriptor, MY_XML_NAME, ts::TID_INT, ts::TID_UNT);

TS_ID_DESCRIPTOR_FACTORY(ts::TargetIPAddressDescriptor, ts::EDID::TableSpecific(MY_DID, ts::TID_INT));
TS_ID_DESCRIPTOR_FACTORY(ts::TargetIPAddressDescriptor, ts::EDID::TableSpecific(MY_DID, ts::TID_UNT));

TS_ID_DESCRIPTOR_DISPLAY(ts::TargetIPAddressDescriptor::DisplayDescriptor, ts::EDID::TableSpecific(MY_DID, ts::TID_INT));
TS_ID_DESCRIPTOR_DISPLAY(ts::TargetIPAddressDescriptor::DisplayDescriptor, ts::EDID::TableSpecific(MY_DID, ts::TID_UNT));


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TargetIPAddressDescriptor::TargetIPAddressDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    IPv4_addr_mask(),
    IPv4_addr()
{
    _is_valid = true;
}

ts::TargetIPAddressDescriptor::TargetIPAddressDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    TargetIPAddressDescriptor()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TargetIPAddressDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt32(IPv4_addr_mask.address());
    for (auto it = IPv4_addr.begin(); it != IPv4_addr.end(); ++it) {
        bbp->appendUInt32(it->address());
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TargetIPAddressDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == _tag && size >= 4 && size % 4 == 0;
    IPv4_addr.clear();

    if (_is_valid) {
        IPv4_addr_mask.setAddress(GetUInt32(data));
        data += 4; size -= 4;
        while (size >= 4) {
            IPv4_addr.push_back(IPAddress(GetUInt32(data)));
            data += 4; size -= 4;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetIPAddressDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    const char* header = "Address mask: ";
    while (size >= 4) {
        strm << margin << header << IPAddress(GetUInt32(data)) << std::endl;
        data += 4; size -= 4;
        header = "Address: ";
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetIPAddressDescriptor::buildXML(xml::Element* root) const
{
    root->setIPAttribute(u"IPv4_addr_mask", IPv4_addr_mask);
    for (auto it = IPv4_addr.begin(); it != IPv4_addr.end(); ++it) {
        root->addElement(u"address")->setIPAttribute(u"IPv4_addr", *it);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::TargetIPAddressDescriptor::fromXML(const xml::Element* element, const DVBCharset* charset)
{
    IPv4_addr.clear();

    xml::ElementVector children;
    _is_valid =
        checkXMLName(element) &&
        element->getIPAttribute(IPv4_addr_mask, u"IPv4_addr_mask", true) &&
        element->getChildren(children, u"address", 0, MAX_ENTRIES);

    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        IPAddress addr;
        _is_valid = children[i]->getIPAttribute(addr, u"IPv4_addr", true);
        if (_is_valid) {
            IPv4_addr.push_back(addr);
        }
    }
}
