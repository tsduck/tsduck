//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMaximumBitrateDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"maximum_bitrate_descriptor"
#define MY_CLASS ts::MaximumBitrateDescriptor
#define MY_DID ts::DID_MAX_BITRATE
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MaximumBitrateDescriptor::MaximumBitrateDescriptor(uint32_t mbr) :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    maximum_bitrate(mbr)
{
}

ts::MaximumBitrateDescriptor::MaximumBitrateDescriptor(DuckContext& duck, const Descriptor& desc) :
    MaximumBitrateDescriptor(0)
{
    deserialize(duck, desc);
}

void ts::MaximumBitrateDescriptor::clearContent()
{
    maximum_bitrate = 0;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MaximumBitrateDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 2);
    buf.putBits(maximum_bitrate, 22);
}

void ts::MaximumBitrateDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(2);
    buf.getBits(maximum_bitrate, 22);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MaximumBitrateDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        buf.skipBits(2);
        const uint32_t mbr = buf.getBits<uint32_t>(22);
        disp << margin << UString::Format(u"Maximum bitrate: 0x%X (%<d), %'d bits/second", {mbr, mbr * BITRATE_UNIT}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MaximumBitrateDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"maximum_bitrate", maximum_bitrate * BITRATE_UNIT, false);
}

bool ts::MaximumBitrateDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    uint32_t mbr = 0;
    bool ok = element->getIntAttribute(mbr, u"maximum_bitrate", true, 0, 0, 0x003FFFFF * BITRATE_UNIT);
    maximum_bitrate = mbr / BITRATE_UNIT;
    return ok;
}
