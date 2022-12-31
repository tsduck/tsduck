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

#include "tsCAContractInfoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"CA_contract_info_descriptor"
#define MY_CLASS ts::CAContractInfoDescriptor
#define MY_DID ts::DID_ISDB_CA_CONTRACT
#define MY_PDS ts::PDS_ISDB
#define MY_STD ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CAContractInfoDescriptor::CAContractInfoDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    CA_system_id(0),
    CA_unit_id(0),
    component_tags(),
    contract_verification_info(),
    fee_name()
{
}

ts::CAContractInfoDescriptor::CAContractInfoDescriptor(DuckContext& duck, const Descriptor& desc) :
    CAContractInfoDescriptor()
{
    deserialize(duck, desc);
}

void ts::CAContractInfoDescriptor::clearContent()
{
    CA_system_id = 0;
    CA_unit_id = 0;
    component_tags.clear();
    contract_verification_info.clear();
    fee_name.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CAContractInfoDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(CA_system_id);
    buf.putBits(CA_unit_id, 4);
    buf.putBits(component_tags.size(), 4);
    buf.putBytes(component_tags);
    buf.putUInt8(uint8_t(contract_verification_info.size()));
    buf.putBytes(contract_verification_info);
    buf.putStringWithByteLength(fee_name);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CAContractInfoDescriptor::deserializePayload(PSIBuffer& buf)
{
    CA_system_id = buf.getUInt16();
    buf.getBits(CA_unit_id, 4);
    const size_t len1 = buf.getBits<size_t>(4);
    buf.getBytes(component_tags, len1);
    const size_t len2 = buf.getUInt8();
    buf.getBytes(contract_verification_info, len2);
    buf.getStringWithByteLength(fee_name);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CAContractInfoDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(5)) {
        disp << margin << "CA System Id: " << names::CASId(disp.duck(), buf.getUInt16(), NamesFlags::FIRST) << std::endl;
        disp << margin << UString::Format(u"CA unit id: %d", {buf.getBits<uint8_t>(4)}) << std::endl;
        for (size_t count = buf.getBits<size_t>(4); buf.canRead() && count > 0; count--) {
            disp << margin << UString::Format(u"Component tag: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        }
        if (buf.canReadBytes(1)) {
            disp.displayPrivateData(u"Contract verification info", buf, buf.getUInt8(), margin);
        }
        if (buf.canReadBytes(1)) {
            disp << margin << "Fee name: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CAContractInfoDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"CA_system_id", CA_system_id, true);
    root->setIntAttribute(u"CA_unit_id", CA_unit_id);
    root->setAttribute(u"fee_name", fee_name, true);
    for (auto it : component_tags) {
        root->addElement(u"component")->setIntAttribute(u"tag", it, true);
    }
    root->addHexaTextChild(u"contract_verification_info", contract_verification_info, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::CAContractInfoDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xcomp;
    bool ok =
        element->getIntAttribute(CA_system_id, u"CA_system_id", true) &&
        element->getIntAttribute(CA_unit_id, u"CA_unit_id", true, 0, 0x00, 0x0F) &&
        element->getAttribute(fee_name, u"fee_name") &&
        element->getChildren(xcomp, u"component", 0, 15) &&
        element->getHexaTextChild(contract_verification_info, u"contract_verification_info", false);

    for (auto it = xcomp.begin(); ok && it != xcomp.end(); ++it) {
        uint8_t tag = 0;
        ok = (*it)->getIntAttribute(tag, u"tag", true);
        component_tags.push_back(tag);
    }
    return ok;
}
