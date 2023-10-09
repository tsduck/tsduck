//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsStreamModeDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"stream_mode_descriptor"
#define MY_CLASS ts::StreamModeDescriptor
#define MY_DID ts::DID_STREAM_MODE
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::StreamModeDescriptor::StreamModeDescriptor(uint8_t mode) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    stream_mode(mode)
{
}

ts::StreamModeDescriptor::StreamModeDescriptor(DuckContext& duck, const Descriptor& desc) :
    StreamModeDescriptor()
{
    deserialize(duck, desc);
}

void ts::StreamModeDescriptor::clearContent()
{
    stream_mode = 0;
}


//----------------------------------------------------------------------------
// Serialization / deserialization.
//----------------------------------------------------------------------------

void ts::StreamModeDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(stream_mode);
    buf.putUInt8(0xFF); // reserved
}

void ts::StreamModeDescriptor::deserializePayload(PSIBuffer& buf)
{
    stream_mode = buf.getUInt8();
    buf.skipBits(8);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::StreamModeDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"Stream mode: %s", {DataName(MY_XML_NAME, u"StreamMode", buf.getUInt8(), NamesFlags::HEXA_FIRST)}) << std::endl;
        buf.skipBits(8);
    }
}


//----------------------------------------------------------------------------
// XML serialization / deserialization.
//----------------------------------------------------------------------------

void ts::StreamModeDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"stream_mode", stream_mode, true);
}

bool ts::StreamModeDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(stream_mode, u"stream_mode", true);
}
