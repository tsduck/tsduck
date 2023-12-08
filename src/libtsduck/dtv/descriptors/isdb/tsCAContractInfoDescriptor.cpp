//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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
