//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsStreamEventDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

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
    event_NPT(npt)
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

void ts::StreamEventDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(event_id);
    buf.putBits(0xFFFFFFFF, 31);
    buf.putBits(event_NPT, 33);
    buf.putBytes(private_data);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::StreamEventDescriptor::deserializePayload(PSIBuffer& buf)
{
    event_id = buf.getUInt16();
    buf.skipBits(31);
    buf.getBits(event_NPT, 33);
    buf.getBytes(private_data);
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

void ts::StreamEventDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(10)) {
        disp << margin << UString::Format(u"Event id: 0x%X (%<d)", {buf.getUInt16()});
        buf.skipBits(31);
        disp << UString::Format(u", NPT: 0x%09X (%<d)", {buf.getBits<uint64_t>(33)}) << std::endl;
        disp.displayPrivateData(u"Private data", buf, NPOS, margin);
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
        element->getIntAttribute(event_id, u"event_id", true, 0, 0x0000, 0xFFFF) &&
        element->getIntAttribute(event_NPT, u"event_NPT", true, 0, 0, 0x00000001FFFFFFFF) &&
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
