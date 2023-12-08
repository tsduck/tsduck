//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsATSCTimeShiftedServiceDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ATSC_time_shifted_service_descriptor"
#define MY_CLASS ts::ATSCTimeShiftedServiceDescriptor
#define MY_DID ts::DID_ATSC_TIME_SHIFT
#define MY_PDS ts::PDS_ATSC
#define MY_STD ts::Standards::ATSC

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ATSCTimeShiftedServiceDescriptor::ATSCTimeShiftedServiceDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::ATSCTimeShiftedServiceDescriptor::clearContent()
{
    entries.clear();
}

ts::ATSCTimeShiftedServiceDescriptor::ATSCTimeShiftedServiceDescriptor(DuckContext& duck, const Descriptor& desc) :
    ATSCTimeShiftedServiceDescriptor()
{
    deserialize(duck, desc);
}

ts::ATSCTimeShiftedServiceDescriptor::Entry::Entry(uint16_t min, uint16_t major, uint16_t minor) :
    time_shift(min),
    major_channel_number(major),
    minor_channel_number(minor)
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ATSCTimeShiftedServiceDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 3);
    buf.putBits(entries.size(), 5);
    for (const auto& it : entries) {
        buf.putBits(0xFF, 6);
        buf.putBits(it.time_shift, 10);
        buf.putBits(0xFF, 4);
        buf.putBits(it.major_channel_number, 10);
        buf.putBits(it.minor_channel_number, 10);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ATSCTimeShiftedServiceDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(3);
    const size_t count = buf.getBits<size_t>(5);
    for (size_t i = 0; i < count && buf.canRead(); ++i) {
        Entry e;
        buf.skipBits(6);
        buf.getBits(e.time_shift, 10);
        buf.skipBits(4);
        buf.getBits(e.major_channel_number, 10);
        buf.getBits(e.minor_channel_number, 10);
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ATSCTimeShiftedServiceDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canRead()) {
        buf.skipBits(3);
        const size_t count = buf.getBits<size_t>(5);
        disp << margin << "Number of services: " << count << std::endl;
        for (size_t i = 0; i < count && buf.canReadBytes(5); ++i) {
            buf.skipBits(6);
            disp << margin << UString::Format(u"- Time shift: %d mn", {buf.getBits<uint16_t>(10)});
            buf.skipBits(4);
            disp << UString::Format(u", service: %d", {buf.getBits<uint16_t>(10)});
            disp << UString::Format(u".%d", {buf.getBits<uint16_t>(10)}) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ATSCTimeShiftedServiceDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"time_shift", it.time_shift, false);
        e->setIntAttribute(u"major_channel_number", it.major_channel_number, false);
        e->setIntAttribute(u"minor_channel_number", it.minor_channel_number, false);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ATSCTimeShiftedServiceDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"service", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getIntAttribute(entry.time_shift, u"time_shift", true, 0, 0, 0x03FF) &&
             children[i]->getIntAttribute(entry.major_channel_number, u"major_channel_number", true, 0, 0, 0x03FF) &&
             children[i]->getIntAttribute(entry.minor_channel_number, u"minor_channel_number", true, 0, 0, 0x03FF);
        entries.push_back(entry);
    }
    return ok;
}
