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

#include "tsTargetMACAddressRangeDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"target_MAC_address_range_descriptor"
#define MY_CLASS ts::TargetMACAddressRangeDescriptor
#define MY_DID ts::DID_INT_MAC_ADDR_RANGE
#define MY_TID ts::TID_INT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TargetMACAddressRangeDescriptor::TargetMACAddressRangeDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    ranges()
{
}

void ts::TargetMACAddressRangeDescriptor::clearContent()
{
    ranges.clear();
}

ts::TargetMACAddressRangeDescriptor::TargetMACAddressRangeDescriptor(DuckContext& duck, const Descriptor& desc) :
    TargetMACAddressRangeDescriptor()
{
    deserialize(duck, desc);
}

ts::TargetMACAddressRangeDescriptor::Range::Range(const MACAddress& addr1, const MACAddress& addr2) :
    MAC_addr_low(addr1),
    MAC_addr_high(addr2)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TargetMACAddressRangeDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    for (auto it = ranges.begin(); it != ranges.end(); ++it) {
        bbp->appendUInt48(it->MAC_addr_low.address());
        bbp->appendUInt48(it->MAC_addr_high.address());
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TargetMACAddressRangeDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size % 12 == 0;
    ranges.clear();

    if (_is_valid) {
        while (size >= 12) {
            ranges.push_back(Range(MACAddress(GetUInt48(data)), MACAddress(GetUInt48(data + 6))));
            data += 12; size -= 12;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetMACAddressRangeDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    while (size >= 12) {
        strm << margin
             << "First address: " << MACAddress(GetUInt48(data))
             << ", last: " << MACAddress(GetUInt48(data + 6))
             << std::endl;
        data += 12; size -= 12;
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetMACAddressRangeDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto it = ranges.begin(); it != ranges.end(); ++it) {
        xml::Element* e = root->addElement(u"range");
        e->setMACAttribute(u"MAC_addr_low", it->MAC_addr_low);
        e->setMACAttribute(u"MAC_addr_high", it->MAC_addr_high);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TargetMACAddressRangeDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"range", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Range range;
        ok = children[i]->getMACAttribute(range.MAC_addr_low, u"MAC_addr_low", true) &&
             children[i]->getMACAttribute(range.MAC_addr_high, u"MAC_addr_high", true);
        ranges.push_back(range);
    }
    return ok;
}
