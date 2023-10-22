//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
#define MY_CLASS ts::ExternalApplicationAuthorizationDescriptor
#define MY_DID ts::DID_AIT_EXT_APP_AUTH
#define MY_TID ts::TID_AIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ExternalApplicationAuthorizationDescriptor::ExternalApplicationAuthorizationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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

void ts::ExternalApplicationAuthorizationDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(7)) {
        disp << margin << UString::Format(u"- Organization id: 0x%X (%<d)", {buf.getUInt32()}) << std::endl;
        disp << margin << UString::Format(u"  Application id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << UString::Format(u"  Priority: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
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
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"application", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getIntAttribute(entry.application_identifier.organization_id, u"organization_id", true) &&
             children[i]->getIntAttribute(entry.application_identifier.application_id, u"application_id", true) &&
             children[i]->getIntAttribute(entry.application_priority, u"application_priority", true);
        entries.push_back(entry);
    }
    return ok;
}
