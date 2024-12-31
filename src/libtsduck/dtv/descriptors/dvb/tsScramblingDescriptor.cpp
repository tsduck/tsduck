//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsScramblingDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"scrambling_descriptor"
#define MY_CLASS    ts::ScramblingDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_DVB_SCRAMBLING, ts::Standards::DVB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ScramblingDescriptor::ScramblingDescriptor(uint8_t mode) :
    AbstractDescriptor(MY_EDID, MY_XML_NAME),
    scrambling_mode(mode)
{
}

void ts::ScramblingDescriptor::clearContent()
{
    scrambling_mode = 0;
}

ts::ScramblingDescriptor::ScramblingDescriptor(DuckContext& duck, const Descriptor& desc) :
    ScramblingDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Binary serialization / deserialization.
//----------------------------------------------------------------------------

void ts::ScramblingDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(scrambling_mode);
}

void ts::ScramblingDescriptor::deserializePayload(PSIBuffer& buf)
{
    scrambling_mode = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ScramblingDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(1)) {
        disp << margin << UString::Format(u"Scrambling mode: %s", NameFromDTV(u"ScramblingMode", buf.getUInt8(), NamesFlags::HEXA_FIRST)) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization / deserialization.
//----------------------------------------------------------------------------

void ts::ScramblingDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"scrambling_mode", scrambling_mode, true);
}

bool ts::ScramblingDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(scrambling_mode, u"scrambling_mode", true);
}
