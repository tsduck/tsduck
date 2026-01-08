//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsExternalApplicationAuthorizationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"external_application_authorization_descriptor"
#define MY_CLASS    ts::ExternalApplicationAuthorizationDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_AIT_EXT_APP_AUTH, ts::Standards::DVB, ts::TID_AIT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ExternalApplicationAuthorizationDescriptor::ExternalApplicationAuthorizationDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::ExternalApplicationAuthorizationDescriptor::ExternalApplicationAuthorizationDescriptor(DuckContext& duck, const Descriptor& desc) :
    ExternalApplicationAuthorizationDescriptor()
{
    deserialize(duck, desc);
}

void ts::ExternalApplicationAuthorizationDescriptor::clearContent()
{
    entries.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ExternalApplicationAuthorizationDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : entries) {
        buf.putUInt32(it.application_identifier.organization_id);
        buf.putUInt16(it.application_identifier.application_id);
        buf.putUInt8(it.application_priority);
    }
}

void ts::ExternalApplicationAuthorizationDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Entry e;
        e.application_identifier.organization_id = buf.getUInt32();
        e.application_identifier.application_id = buf.getUInt16();
        e.application_priority = buf.getUInt8();
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ExternalApplicationAuthorizationDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    while (buf.canReadBytes(7)) {
        disp << margin << UString::Format(u"- Organization id: %n", buf.getUInt32()) << std::endl;
        disp << margin << UString::Format(u"  Application id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"  Priority: %n", buf.getUInt8()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ExternalApplicationAuthorizationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"application");
        e->setIntAttribute(u"organization_id", it.application_identifier.organization_id, true);
        e->setIntAttribute(u"application_id", it.application_identifier.application_id, true);
        e->setIntAttribute(u"application_priority", it.application_priority, false);
    }
}

bool ts::ExternalApplicationAuthorizationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = true;
    for (auto& child : element->children(u"application", &ok, 0, MAX_ENTRIES)) {
        auto& entry(entries.emplace_back());
        ok = child.getIntAttribute(entry.application_identifier.organization_id, u"organization_id", true) &&
             child.getIntAttribute(entry.application_identifier.application_id, u"application_id", true) &&
             child.getIntAttribute(entry.application_priority, u"application_priority", true);
    }
    return ok;
}
