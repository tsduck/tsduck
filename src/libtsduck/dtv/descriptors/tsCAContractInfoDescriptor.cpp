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

#include "tsCAContractInfoDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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

void ts::CAContractInfoDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt16(CA_system_id);
    bbp->appendUInt8(uint8_t(CA_unit_id << 4) | uint8_t(component_tags.size() & 0x0F));
    bbp->append(component_tags);
    bbp->appendUInt8(uint8_t(contract_verification_info.size()));
    bbp->append(contract_verification_info);
    bbp->append(duck.encodedWithByteLength(fee_name));
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CAContractInfoDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    component_tags.clear();
    contract_verification_info.clear();
    fee_name.clear();

    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() >= 5;

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        CA_system_id = GetUInt16(data);
        CA_unit_id = (data[2] >> 4) & 0x0F;
        const size_t len1 = data[2] & 0x0F;
        data += 3; size -= 3;
        _is_valid = size >= len1 + 2;
        if (_is_valid) {
            component_tags.copy(data, len1);
            const size_t len2 = data[len1];
            data += len1 + 1; size -= len1 + 1;
            _is_valid = size >= len2 + 1;
            if (_is_valid) {
                contract_verification_info.copy(data, len2);
                data += len2; size -= len2;
                duck.decodeWithByteLength(fee_name, data, size);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CAContractInfoDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 5) {
        size_t count = data[2] & 0x0F;
        strm << margin << "CA System Id: " << names::CASId(duck, GetUInt16(data), names::FIRST) << std::endl
             << margin << UString::Format(u"CA unit id: %d", {(data[2] >> 4) & 0x0F}) << std::endl;
        data += 3; size -= 3;
        while (size > 0 && count > 0) {
            strm << margin << UString::Format(u"Component tag: 0x%X (%d)", {data[0], data[0]}) << std::endl;
            data++; size--; count--;
        }
        if (size > 0) {
            count = std::min<size_t>(data[0], size - 1);
            display.displayPrivateData(u"Contract verification info", data + 1, count, indent);
            data += count + 1; size -= count + 1;
        }
        if (size > 0) {
            strm << margin << "Fee name: \"" << duck.decodedWithByteLength(data, size) << "\"" << std::endl;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CAContractInfoDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"CA_system_id", CA_system_id, true);
    root->setIntAttribute(u"CA_unit_id", CA_unit_id);
    root->setAttribute(u"fee_name", fee_name, true);
    for (auto it = component_tags.begin(); it != component_tags.end(); ++it) {
        root->addElement(u"component")->setIntAttribute(u"tag", *it, true);
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
        element->getIntAttribute<uint16_t>(CA_system_id, u"CA_system_id", true) &&
        element->getIntAttribute<uint8_t>(CA_unit_id, u"CA_unit_id", true, 0, 0x00, 0x0F) &&
        element->getAttribute(fee_name, u"fee_name") &&
        element->getChildren(xcomp, u"component", 0, 15) &&
        element->getHexaTextChild(contract_verification_info, u"contract_verification_info", false);

    for (auto it = xcomp.begin(); ok && it != xcomp.end(); ++it) {
        uint8_t tag = 0;
        ok = (*it)->getIntAttribute<uint8_t>(tag, u"tag", true);
        component_tags.push_back(tag);
    }
    return ok;
}
