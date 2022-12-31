//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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

ts::EASInbandExceptionChannelsDescriptor::Entry::Entry(uint8_t chan, uint16_t prog) :
    RF_channel(chan),
    program_number(prog)
{
}

void ts::EASInbandExceptionChannelsDescriptor::clearContent()
{
    entries.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::EASInbandExceptionChannelsDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(uint8_t(entries.size()));
    for (const auto& it : entries) {
        buf.putUInt8(it.RF_channel);
        buf.putUInt16(it.program_number);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::EASInbandExceptionChannelsDescriptor::deserializePayload(PSIBuffer& buf)
{
    const uint8_t count = buf.getUInt8();
    for (size_t i = 0; i < count && buf.canRead(); ++i) {
        Entry e;
        e.RF_channel = buf.getUInt8();
        e.program_number = buf.getUInt16();
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::EASInbandExceptionChannelsDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        uint8_t count = buf.getUInt8();
        disp << margin << UString::Format(u"Exception channel count: %d", {count}) << std::endl;
        while (buf.canReadBytes(3) && count-- > 0) {
            disp << margin << UString::Format(u"  RF channel: %d", {buf.getUInt8()});
            disp << UString::Format(u", program number 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::EASInbandExceptionChannelsDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"exception");
        e->setIntAttribute(u"RF_channel", it.RF_channel, false);
        e->setIntAttribute(u"program_number", it.program_number, true);
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
        ok = children[i]->getIntAttribute(entry.RF_channel, u"RF_channel", true) &&
             children[i]->getIntAttribute(entry.program_number, u"program_number", true);
        entries.push_back(entry);
    }
    return ok;
}
