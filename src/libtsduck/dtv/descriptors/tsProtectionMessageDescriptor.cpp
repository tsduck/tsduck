//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsProtectionMessageDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"protection_message_descriptor"
#define MY_CLASS ts::ProtectionMessageDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_PROTECTION_MSG
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ProtectionMessageDescriptor::ProtectionMessageDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::ProtectionMessageDescriptor::clearContent()
{
    component_tags.clear();
}

ts::ProtectionMessageDescriptor::ProtectionMessageDescriptor(DuckContext& duck, const Descriptor& desc) :
    ProtectionMessageDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::ProtectionMessageDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ProtectionMessageDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 4);
    buf.putBits(component_tags.size(), 4);
    buf.putBytes(component_tags);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ProtectionMessageDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(4);
    const size_t count = buf.getBits<size_t>(4);
    buf.getBytes(component_tags, count);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ProtectionMessageDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        buf.skipBits(4);
        size_t count = buf.getBits<size_t>(4);
        disp << margin << UString::Format(u"Component count: %d", {count}) << std::endl;
        while (count > 0 && buf.canReadBytes(1)) {
            disp << margin << UString::Format(u"Component tag: 0x%0X (%<d)", {buf.getUInt8()}) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ProtectionMessageDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (size_t i = 0; i < component_tags.size(); ++i) {
        root->addElement(u"component")->setIntAttribute(u"tag", component_tags[i], true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ProtectionMessageDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"component", 0, 15);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        uint8_t t = 0;
        ok = children[i]->getIntAttribute(t, u"tag", true);
        component_tags.push_back(t);
    }
    return ok;
}
