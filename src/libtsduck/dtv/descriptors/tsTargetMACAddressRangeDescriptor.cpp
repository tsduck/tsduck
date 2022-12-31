//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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

void ts::TargetMACAddressRangeDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : ranges) {
        buf.putUInt48(it.MAC_addr_low.address());
        buf.putUInt48(it.MAC_addr_high.address());
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TargetMACAddressRangeDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Range r;
        r.MAC_addr_low.setAddress(buf.getUInt48());
        r.MAC_addr_high.setAddress(buf.getUInt48());
        ranges.push_back(r);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TargetMACAddressRangeDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(12)) {
        disp << margin << "First address: " << MACAddress(buf.getUInt48());
        disp << ", last: " << MACAddress(buf.getUInt48()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TargetMACAddressRangeDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : ranges) {
        xml::Element* e = root->addElement(u"range");
        e->setMACAttribute(u"MAC_addr_low", it.MAC_addr_low);
        e->setMACAttribute(u"MAC_addr_high", it.MAC_addr_high);
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
