//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCAIdentifierDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"CA_identifier_descriptor"
#define MY_CLASS ts::CAIdentifierDescriptor
#define MY_DID ts::DID_CA_ID
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::CAIdentifierDescriptor::CAIdentifierDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::CAIdentifierDescriptor::clearContent()
{
    casids.clear();
}

ts::CAIdentifierDescriptor::CAIdentifierDescriptor(DuckContext& duck, const Descriptor& desc) :
    CAIdentifierDescriptor()
{
    deserialize(duck, desc);
}

ts::CAIdentifierDescriptor::CAIdentifierDescriptor(std::initializer_list<uint16_t> ids) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    casids(ids)
{
}


//----------------------------------------------------------------------------
// Exactly identical descriptors shall not be dumplicated.
//----------------------------------------------------------------------------

ts::DescriptorDuplication ts::CAIdentifierDescriptor::duplicationMode() const
{
    return DescriptorDuplication::ADD_OTHER;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CAIdentifierDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (size_t n = 0; n < casids.size(); ++n) {
        buf.putUInt16(casids[n]);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CAIdentifierDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        casids.push_back(buf.getUInt16());
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CAIdentifierDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(2)) {
        disp << margin << "CA System Id: " << names::CASId(disp.duck(), buf.getUInt16(), NamesFlags::FIRST) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CAIdentifierDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (size_t i = 0; i < casids.size(); ++i) {
        root->addElement(u"CA_system_id")->setIntAttribute(u"value", casids[i], true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::CAIdentifierDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"CA_system_id", 0, (MAX_DESCRIPTOR_SIZE - 2) / 2);
    for (size_t i = 0; ok && i < children.size(); ++i) {
        uint16_t id = 0;
        ok = children[i]->getIntAttribute(id, u"value", true, 0, 0x0000, 0xFFFF);
        casids.push_back(id);
    }
    return ok;
}
