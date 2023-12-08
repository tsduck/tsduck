//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsServiceLocationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"service_location_descriptor"
#define MY_CLASS ts::ServiceLocationDescriptor
#define MY_DID ts::DID_ATSC_SERVICE_LOC
#define MY_PDS ts::PDS_ATSC
#define MY_STD ts::Standards::ATSC

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ServiceLocationDescriptor::ServiceLocationDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::ServiceLocationDescriptor::clearContent()
{
    PCR_PID = PID_NULL;
    entries.clear();
}

ts::ServiceLocationDescriptor::ServiceLocationDescriptor(DuckContext& duck, const Descriptor& desc) :
    ServiceLocationDescriptor()
{
    deserialize(duck, desc);
}

ts::ServiceLocationDescriptor::Entry::Entry(uint8_t type, ts::PID pid, const ts::UString& lang) :
    stream_type(type),
    elementary_PID(pid),
    ISO_639_language_code(lang)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ServiceLocationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putPID(PCR_PID);
    buf.putUInt8(uint8_t(entries.size()));
    for (const auto& it : entries) {
        buf.putUInt8(it.stream_type);
        buf.putPID(it.elementary_PID);
        buf.putLanguageCode(it.ISO_639_language_code, true);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ServiceLocationDescriptor::deserializePayload(PSIBuffer& buf)
{
    PCR_PID = buf.getPID();
    const size_t count = buf.getUInt8();
    for (size_t i = 0; i < count && buf.canRead(); ++i) {
        Entry e;
        e.stream_type = buf.getUInt8();
        e.elementary_PID = buf.getPID();
        buf.getLanguageCode(e.ISO_639_language_code);
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ServiceLocationDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        const PID pid = buf.getPID();
        const size_t count = buf.getUInt8();
        disp << margin << "PCR PID: ";
        if (pid == PID_NULL) {
            disp << "none";
        }
        else {
            disp << UString::Format(u"0x%X (%<d)", {pid});
        }
        disp << ", number of elements: " << count << std::endl;

        // Loop on all component entries.
        for (size_t i = 0; i < count && buf.canReadBytes(6); ++i) {
            const uint8_t stype = buf.getUInt8();
            disp << margin << UString::Format(u"- PID: 0x%X (%<d)", {buf.getPID()});
            disp << ", language: \"" << buf.getLanguageCode() << "\", type: " << names::ServiceType(stype, NamesFlags::FIRST) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ServiceLocationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    if (PCR_PID != PID_NULL) {
        root->setIntAttribute(u"PCR_PID", PCR_PID, true);
    }
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"component");
        e->setIntAttribute(u"stream_type", it.stream_type, true);
        e->setIntAttribute(u"elementary_PID", it.elementary_PID, true);
        if (!it.ISO_639_language_code.empty()) {
            e->setAttribute(u"ISO_639_language_code", it.ISO_639_language_code);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ServiceLocationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok =
        element->getIntAttribute(PCR_PID, u"PCR_PID", false, PID_NULL, 0, 0x1FFF) &&
        element->getChildren(children, u"component", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getIntAttribute(entry.stream_type, u"stream_type", true) &&
             children[i]->getIntAttribute(entry.elementary_PID, u"elementary_PID", true, 0, 0, 0x1FFF) &&
             children[i]->getAttribute(entry.ISO_639_language_code, u"ISO_639_language_code", false, UString(), 0, 3);
        entries.push_back(entry);
    }
    return ok;
}
