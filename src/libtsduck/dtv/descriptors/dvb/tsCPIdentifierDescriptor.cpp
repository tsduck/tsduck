//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCPIdentifierDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"CP_identifier_descriptor"
#define MY_CLASS    ts::CPIdentifierDescriptor
#define MY_EDID     ts::EDID::ExtensionDVB(ts::XDID_DVB_CP_IDENTIFIER)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::CPIdentifierDescriptor::CPIdentifierDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::CPIdentifierDescriptor::clearContent()
{
    cpids.clear();
}

ts::CPIdentifierDescriptor::CPIdentifierDescriptor(DuckContext& duck, const Descriptor& desc) :
    CPIdentifierDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CPIdentifierDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (size_t n = 0; n < cpids.size(); ++n) {
        buf.putUInt16(cpids[n]);
    }
}

void ts::CPIdentifierDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        cpids.push_back(buf.getUInt16());
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::CPIdentifierDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (size_t i = 0; i < cpids.size(); ++i) {
        root->addElement(u"CP_system_id")->setIntAttribute(u"value", cpids[i], true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::CPIdentifierDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = true;
    for (auto& child : element->children(u"CP_system_id", &ok, 0, (MAX_DESCRIPTOR_SIZE - 3) / 2)) {
        auto& id(cpids.emplace_back());
        ok = child.getIntAttribute(id, u"value", true);
    }
    return ok;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::CPIdentifierDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    while (buf.canReadBytes(2)) {
        disp << margin << "CP System Id: " << DataName(MY_XML_NAME, u"CPSystemId", buf.getUInt16(), NamesFlags::VALUE_NAME) << std::endl;
    }
}
