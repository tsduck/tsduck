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

#include "tsApplicationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    profiles(),
    service_bound(false),
    visibility(0),
    application_priority(0),
    transport_protocol_labels()
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

ts::ApplicationDescriptor::Profile::Profile() :
    application_profile(0),
    version_major(0),
    version_minor(0),
    version_micro(0)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ApplicationDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(5 * profiles.size()));
    for (auto it = profiles.begin(); it != profiles.end(); ++it) {
        bbp->appendUInt16(it->application_profile);
        bbp->appendUInt8(it->version_major);
        bbp->appendUInt8(it->version_minor);
        bbp->appendUInt8(it->version_micro);
    }
    bbp->appendUInt8((service_bound ? 0x80 : 0x00) | uint8_t((visibility & 0x03) << 5) | 0x1F);
    bbp->appendUInt8(application_priority);
    bbp->append(transport_protocol_labels);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ApplicationDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    profiles.clear();
    transport_protocol_labels.clear();

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1;
    size_t profiles_count = 0;

    if (_is_valid) {
        _is_valid = data[0] % 5 == 0 && size >= size_t(1 + data[0]);
        profiles_count = data[0] / 5;
        data++; size--;
    }
    while (_is_valid && profiles_count-- > 0) {
        Profile p;
        p.application_profile = GetUInt16(data);
        p.version_major = data[2];
        p.version_minor = data[3];
        p.version_micro = data[4];
        data += 5; size -= 5;
        profiles.push_back(p);
    }
    _is_valid = _is_valid && size >= 2;
    if (_is_valid) {
        service_bound = (data[0] & 0x80) != 0;
        visibility = (data[0] >> 5) & 0x03;
        application_priority = data[1];
        transport_protocol_labels.copy(data + 2, size - 2);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ApplicationDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        size_t len = std::min<size_t>(data[0], size - 1);
        data++; size--;
        while (size >= 5 && len >= 5) {
            const uint16_t pr = GetUInt16(data);
            strm << margin << UString::Format(u"Profile: 0x%X (%d), version: %d.%d.%d", {pr, pr, data[2], data[3], data[4]}) << std::endl;
            data += 5; size -= 5; len -= 5;
        }
        if (size >= 1) {
            strm << margin
                 << UString::Format(u"Service bound: %d, visibility: %d, priority: %d", {data[0] >> 7, (data[0] >> 5) & 0x03, data[1]})
                 << std::endl;
            data += 2; size -= 2;
        }
        while (size > 0) {
            strm << margin << UString::Format(u"Transport protocol label: 0x%X (%d)", {data[0], data[0]}) << std::endl;
            data++; size--;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ApplicationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"service_bound", service_bound);
    root->setIntAttribute(u"visibility", visibility);
    root->setIntAttribute(u"application_priority", application_priority);
    for (auto it = profiles.begin(); it != profiles.end(); ++it) {
        xml::Element* e = root->addElement(u"profile");
        e->setIntAttribute(u"application_profile", it->application_profile, true);
        e->setAttribute(u"version", UString::Format(u"%d.%d.%d", {it->version_major, it->version_minor, it->version_micro}));
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
        element->getIntAttribute<uint8_t>(visibility, u"visibility", true, 0, 0, 3) &&
        element->getIntAttribute<uint8_t>(application_priority, u"application_priority", true) &&
        element->getChildren(prof, u"profile") &&
        element->getChildren(label, u"transport_protocol");

    for (size_t i = 0; ok && i < prof.size(); ++i) {
        Profile p;
        UString version;
        ok = prof[i]->getIntAttribute<uint16_t>(p.application_profile, u"application_profile", true) &&
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
        ok = label[i]->getIntAttribute<uint8_t>(l, u"label", true);
        if (ok) {
            transport_protocol_labels.push_back(l);
        }
    }
    return ok;
}
