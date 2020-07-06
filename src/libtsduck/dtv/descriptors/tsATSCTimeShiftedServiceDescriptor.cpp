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

#include "tsATSCTimeShiftedServiceDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    entries()
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

void ts::ATSCTimeShiftedServiceDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(0xE0 | (entries.size() & 0x1F)));
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        bbp->appendUInt16(uint16_t(0xFC00 | (it->time_shift & 0x03FF)));
        bbp->appendUInt24(0xF00000 | (uint32_t(it->major_channel_number & 0x03FF) << 10) | (it->minor_channel_number & 0x03FF));
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ATSCTimeShiftedServiceDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    entries.clear();

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 1;

    if (_is_valid) {
        size_t count = data[0] & 0x1F;
        data++; size--;
        while (size >= 5 && count > 0) {
            const uint32_t n = GetUInt24(data + 2);
            entries.push_back(Entry(GetUInt16(data) & 0x03FF, uint16_t((n >> 10) & 0x03FF), uint16_t(n & 0x03FF)));
            data += 5; size -= 5; count--;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ATSCTimeShiftedServiceDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        size_t count = data[0] & 0x1F;
        data++; size--;
        strm << margin << "Number of services: " << count << std::endl;
        while (size >= 5 && count > 0) {
            const uint32_t n = GetUInt24(data + 2);
            strm << margin << UString::Format(u"- Time shift: %d mn, service: %d.%d", {GetUInt16(data) & 0x03FF, (n >> 10) & 0x03FF, n & 0x03FF}) << std::endl;
            data += 5; size -= 5; count--;
        }

    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ATSCTimeShiftedServiceDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"service");
        e->setIntAttribute(u"time_shift", it->time_shift, false);
        e->setIntAttribute(u"major_channel_number", it->major_channel_number, false);
        e->setIntAttribute(u"minor_channel_number", it->minor_channel_number, false);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ATSCTimeShiftedServiceDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"service", 0, MAX_ENTRIES);

    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getIntAttribute<uint16_t>(entry.time_shift, u"time_shift", true, 0, 0, 0x03FF) &&
             children[i]->getIntAttribute<uint16_t>(entry.major_channel_number, u"major_channel_number", true, 0, 0, 0x03FF) &&
             children[i]->getIntAttribute<uint16_t>(entry.minor_channel_number, u"minor_channel_number", true, 0, 0, 0x03FF);
        entries.push_back(entry);
    }
    return ok;
}
