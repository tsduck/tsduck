//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMultiplexBufferDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"multiplex_buffer_descriptor"
#define MY_CLASS ts::MultiplexBufferDescriptor
#define MY_DID ts::DID_MUX_BUFFER
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MultiplexBufferDescriptor::MultiplexBufferDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::MultiplexBufferDescriptor::clearContent()
{
    MB_buffer_size = 0;
    TB_leak_rate = 0;
}

ts::MultiplexBufferDescriptor::MultiplexBufferDescriptor(DuckContext& duck, const Descriptor& desc) :
    MultiplexBufferDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MultiplexBufferDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt24(MB_buffer_size);
    buf.putUInt24(TB_leak_rate);
}

void ts::MultiplexBufferDescriptor::deserializePayload(PSIBuffer& buf)
{
    MB_buffer_size = buf.getUInt24();
    TB_leak_rate = buf.getUInt24();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MultiplexBufferDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(6)) {
        disp << margin << UString::Format(u"MB buffer size: %'d bytes", {buf.getUInt24()}) << std::endl;
        const uint32_t tb = buf.getUInt24();
        disp << margin << UString::Format(u"TB leak rate: %'d (%'d bits/s)", {tb, 400 * tb}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MultiplexBufferDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"MB_buffer_size", MB_buffer_size);
    root->setIntAttribute(u"TB_leak_rate", TB_leak_rate);
}

bool ts::MultiplexBufferDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(MB_buffer_size, u"MB_buffer_size", true, 0, 0, 0x00FFFFFF) &&
           element->getIntAttribute(TB_leak_rate, u"TB_leak_rate", true, 0, 0, 0x00FFFFFF);
}
