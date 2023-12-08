//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsApplicationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"application_descriptor"
#define MY_CLASS ts::ApplicationDescriptor
#define MY_DID ts::DID_AIT_APPLICATION
#define MY_TID ts::TID_AIT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ApplicationDescriptor::ApplicationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::ApplicationDescriptor::clearContent()
{
    profiles.clear();
    service_bound = false;
    visibility = 0;
    application_priority = 0;
    transport_protocol_labels.clear();
}

ts::ApplicationDescriptor::ApplicationDescriptor(DuckContext& duck, const Descriptor& desc) :
    ApplicationDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ApplicationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.pushWriteSequenceWithLeadingLength(8); // application_profiles_length
    for (const auto& it : profiles) {
        buf.putUInt16(it.application_profile);
        buf.putUInt8(it.version_major);
        buf.putUInt8(it.version_minor);
        buf.putUInt8(it.version_micro);
    }
    buf.popState(); // update application_profiles_length
    buf.putBit(service_bound);
    buf.putBits(visibility, 2);
    buf.putBits(0xFF, 5);
    buf.putUInt8(application_priority);
    buf.putBytes(transport_protocol_labels);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ApplicationDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.pushReadSizeFromLength(8); // application_profiles_length
    while (buf.canRead()) {
        Profile p;
        p.application_profile = buf.getUInt16();
        p.version_major = buf.getUInt8();
        p.version_minor = buf.getUInt8();
        p.version_micro = buf.getUInt8();
        profiles.push_back(p);
    }
    buf.popState(); // end of application_profiles_length
    service_bound = buf.getBool();
    buf.getBits(visibility, 2);
    buf.skipBits(5);
    application_priority = buf.getUInt8();
    buf.getBytes(transport_protocol_labels);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ApplicationDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    buf.pushReadSizeFromLength(8); // application_profiles_length
    while (buf.canReadBytes(5)) {
        disp << margin << UString::Format(u"Profile: 0x%X (%<d)", {buf.getUInt16()});
        disp << UString::Format(u", version: %d", {buf.getUInt8()});
        disp << UString::Format(u".%d", {buf.getUInt8()});
        disp << UString::Format(u".%d", {buf.getUInt8()}) << std::endl;
    }
    buf.popState(); // end of application_profiles_length
    if (buf.canReadBytes(1)) {
        disp << margin << UString::Format(u"Service bound: %d", {buf.getBool()});
        disp << UString::Format(u", visibility: %d", {buf.getBits<uint8_t>(2)});
        buf.skipBits(5);
        disp << UString::Format(u", priority: %d", {buf.getUInt8()}) << std::endl;
    }
    while (buf.canReadBytes(1)) {
        disp << margin << UString::Format(u"Transport protocol label: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ApplicationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"service_bound", service_bound);
    root->setIntAttribute(u"visibility", visibility);
    root->setIntAttribute(u"application_priority", application_priority);
    for (const auto& it : profiles) {
        xml::Element* e = root->addElement(u"profile");
        e->setIntAttribute(u"application_profile", it.application_profile, true);
        e->setAttribute(u"version", UString::Format(u"%d.%d.%d", {it.version_major, it.version_minor, it.version_micro}));
    }
    for (size_t i = 0; i < transport_protocol_labels.size(); ++i) {
        root->addElement(u"transport_protocol")->setIntAttribute(u"label", transport_protocol_labels[i], true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ApplicationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector prof;
    xml::ElementVector label;
    bool ok =
        element->getBoolAttribute(service_bound, u"service_bound", true) &&
        element->getIntAttribute(visibility, u"visibility", true, 0, 0, 3) &&
        element->getIntAttribute(application_priority, u"application_priority", true) &&
        element->getChildren(prof, u"profile") &&
        element->getChildren(label, u"transport_protocol");

    for (size_t i = 0; ok && i < prof.size(); ++i) {
        Profile p;
        UString version;
        ok = prof[i]->getIntAttribute(p.application_profile, u"application_profile", true) &&
             prof[i]->getAttribute(version, u"version", true);
        if (ok && !version.scan(u"%d.%d.%d", {&p.version_major, &p.version_minor, &p.version_micro})) {
            ok = false;
            prof[i]->report().error(u"invalid version '%s' in <%s>, line %d, use 'major.minor.micro'", {version, prof[i]->name(), prof[i]->lineNumber()});
        }
        if (ok) {
            profiles.push_back(p);
        }
    }
    for (size_t i = 0; ok && i < label.size(); ++i) {
        uint8_t l;
        ok = label[i]->getIntAttribute(l, u"label", true);
        if (ok) {
            transport_protocol_labels.push_back(l);
        }
    }
    return ok;
}
