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

#include "tsStreamEventDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"stream_event_descriptor"
#define MY_CLASS ts::StreamEventDescriptor
#define MY_DID ts::DID_STREAM_EVENT
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::StreamEventDescriptor::StreamEventDescriptor(uint16_t id, uint64_t npt) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    event_id(id),
    event_NPT(npt),
    private_data()
{
}

void ts::StreamEventDescriptor::clearContent()
{
    event_id = 0;
    event_NPT = 0;
    private_data.clear();
}

ts::StreamEventDescriptor::StreamEventDescriptor(DuckContext& duck, const Descriptor& desc) :
    StreamEventDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::StreamEventDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt16(event_id);
    bbp->appendUInt64(TS_UCONST64(0xFFFFFFFE00000000) | event_NPT);
    bbp->append(private_data);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::StreamEventDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() >= 10;

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        event_id = GetUInt16(data);
        event_NPT = GetUInt64(data + 2) & TS_UCONST64(0x00000001FFFFFFFF);
        private_data.copy (data + 10, size - 10);
    }
}


//----------------------------------------------------------------------------
// Check if all bytes in private part are ASCII characters.
//----------------------------------------------------------------------------

bool ts::StreamEventDescriptor::asciiPrivate() const
{
    bool ascii = !private_data.empty();
    for (size_t i = 0; ascii && i < private_data.size(); ++i) {
        ascii = private_data[i] >= 0x20 && private_data[i] < 0x80;
    }
    return ascii;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::StreamEventDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 10) {
        const uint16_t id = GetUInt16(data);
        const uint64_t npt = GetUInt64(data + 2) & TS_UCONST64(0x00000001FFFFFFFF);
        strm << margin << UString::Format(u"Event id: 0x%X (%d), NPT: 0x%09X (%d)", {id, id, npt, npt}) << std::endl;
        display.displayPrivateData(u"Private data", data + 10, size - 10, indent);
    }
    else {
        display.displayExtraData(data, size, indent);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::StreamEventDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"event_id", event_id, true);
    root->setIntAttribute(u"event_NPT", event_NPT, true);
    if (!private_data.empty()) {
        if (asciiPrivate()) {
            root->addElement(u"private_text")->addText(UString::FromUTF8(reinterpret_cast<const char*>(private_data.data()), private_data.size()));
        }
        else {
            root->addHexaTextChild(u"private_data", private_data);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::StreamEventDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    UString text;
    bool ok =
        element->getIntAttribute<uint16_t>(event_id, u"event_id", true, 0, 0x0000, 0xFFFF) &&
        element->getIntAttribute<uint64_t>(event_NPT, u"event_NPT", true, 0, 0, TS_UCONST64(0x00000001FFFFFFFF)) &&
        element->getHexaTextChild(private_data, u"private_data", false, 0, MAX_DESCRIPTOR_SIZE - 10) &&
        element->getTextChild(text, u"private_text", false, false, UString(), 0, MAX_DESCRIPTOR_SIZE - 10);

    if (ok && !text.empty()) {
        if (private_data.empty()) {
            private_data.appendUTF8(text);
        }
        else {
            element->report().error(u"In <%s> at line %d, <private_data> and <private_text> are mutually exclusive", {element->name(), element->lineNumber()});
        }
    }
    return ok;
}
