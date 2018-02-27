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

#include "tsTargetMACAddressDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"target_MAC_address_descriptor"
#define MY_DID ts::DID_INT_MAC_ADDR
#define MY_TID ts::TID_INT

TS_XML_TABSPEC_DESCRIPTOR_FACTORY(ts::TargetMACAddressDescriptor, MY_XML_NAME, MY_TID);
TS_ID_DESCRIPTOR_FACTORY(ts::TargetMACAddressDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));
TS_ID_DESCRIPTOR_DISPLAY(ts::TargetMACAddressDescriptor::DisplayDescriptor, ts::EDID::TableSpecific(MY_DID, MY_TID));


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TargetMACAddressDescriptor::TargetMACAddressDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME),
    MAC_addr_mask(),
    MAC_addr()
{
    _is_valid = true;
}

ts::TargetMACAddressDescriptor::TargetMACAddressDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    TargetMACAddressDescriptor()
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TargetMACAddressDescriptor::serialize(Descriptor& desc, const DVBCharset* charset) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt48(MAC_addr_mask.address());
    for (auto it = MAC_addr.begin(); it != MAC_addr.end(); ++it) {
        bbp->appendUInt48(it->address());
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TargetMACAddressDescriptor::deserialize(const Descriptor& desc, const DVBCharset* charset)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == _tag && size >= 6 && size % 6 == 0;
    MAC_addr.clear();

    if (_is_valid) {
        MAC_addr_mask.setAddress(GetUInt48(data));
        data += 6; size -= 6;
        while (size >= 6) {
            MAC_addr.push_back(MACAddress(GetUInt48(data)));
            data += 6; size -= 6;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetMACAddressDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    const char* header = "Address mask: ";
    while (size >= 6) {
        strm << margin << header << MACAddress(GetUInt48(data)).toString() << std::endl;
        data += 6; size -= 6;
        header = "Address: ";
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetMACAddressDescriptor::buildXML(xml::Element* root) const
{
    root->setMACAttribute(u"MAC_addr_mask", MAC_addr_mask);
    for (auto it = MAC_addr.begin(); it != MAC_addr.end(); ++it) {
        root->addElement(u"address")->setMACAttribute(u"MAC_addr", *it);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::TargetMACAddressDescriptor::fromXML(const xml::Element* element)
{
    MAC_addr.clear();

    xml::ElementVector children;
    _is_valid =
        checkXMLName(element) &&
        element->getMACAttribute(MAC_addr_mask, u"MAC_addr_mask", true) &&
        element->getChildren(children, u"address", 0, MAX_ENTRIES);

    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        MACAddress addr;
        _is_valid = children[i]->getMACAttribute(addr, u"MAC_addr", true);
        if (_is_valid) {
            MAC_addr.push_back(addr);
        }
    }
}
