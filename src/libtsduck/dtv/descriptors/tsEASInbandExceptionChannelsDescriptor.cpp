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

#include "tsEASInbandExceptionChannelsDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"EAS_inband_exception_channels_descriptor"
#define MY_CLASS ts::EASInbandExceptionChannelsDescriptor
#define MY_DID ts::DID_EAS_INBAND_EXCEPTS
#define MY_TID ts::TID_SCTE18_EAS
#define MY_STD ts::Standards::SCTE

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
const size_t ts::EASInbandExceptionChannelsDescriptor::MAX_ENTRIES;
#endif


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::EASInbandExceptionChannelsDescriptor::EASInbandExceptionChannelsDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    entries()
{
}

ts::EASInbandExceptionChannelsDescriptor::EASInbandExceptionChannelsDescriptor(DuckContext& duck, const Descriptor& desc) :
    EASInbandExceptionChannelsDescriptor()
{
    deserialize(duck, desc);
}

void ts::EASInbandExceptionChannelsDescriptor::clearContent()
{
    entries.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::EASInbandExceptionChannelsDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    size_t count = std::min(entries.size(), MAX_ENTRIES);
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(uint8_t(count));
    for (auto it = entries.begin(); count > 0 && it != entries.end(); ++it, --count) {
        bbp->appendUInt8(it->RF_channel);
        bbp->appendUInt16(it->program_number);
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::EASInbandExceptionChannelsDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    _is_valid = desc.isValid() && desc.tag() == tag() && size > 0 && (size - 1) % 3 == 0;
    entries.clear();

    if (_is_valid) {
        uint8_t count = data[0];
        data++; size--;
        while (size >= 3 && count-- > 0) {
            entries.push_back(Entry(data[0], GetUInt16(data + 1)));
            data += 3; size -= 3;
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::EASInbandExceptionChannelsDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size > 0) {
        uint8_t count = data[0];
        data++; size--;
        strm << margin << UString::Format(u"Exception channel count: %d", {count}) << std::endl;
        while (size >= 3 && count-- > 0) {
            strm << margin << UString::Format(u"  RF channel: %d, program number 0x%X (%d)", {data[0], GetUInt16(data + 1), GetUInt16(data + 1)}) << std::endl;
            data += 3; size -= 3;
        }
    }

    display.displayExtraData(data, size, indent);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::EASInbandExceptionChannelsDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        xml::Element* e = root->addElement(u"exception");
        e->setIntAttribute(u"RF_channel", it->RF_channel, false);
        e->setIntAttribute(u"program_number", it->program_number, true);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::EASInbandExceptionChannelsDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"exception", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry entry;
        ok = children[i]->getIntAttribute<uint8_t>(entry.RF_channel, u"RF_channel", true) &&
             children[i]->getIntAttribute<uint16_t>(entry.program_number, u"program_number", true);
        entries.push_back(entry);
    }
    return ok;
}
