//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEASInbandDetailsChannelDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"EAS_inband_details_channel_descriptor"
#define MY_CLASS ts::EASInbandDetailsChannelDescriptor
#define MY_DID ts::DID_EAS_INBAND_DETAILS
#define MY_TID ts::TID_SCTE18_EAS
#define MY_STD ts::Standards::SCTE

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::TableSpecific(MY_DID, MY_TID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::EASInbandDetailsChannelDescriptor::EASInbandDetailsChannelDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::EASInbandDetailsChannelDescriptor::clearContent()
{
    details_RF_channel = 0;
    details_program_number = 0;
}

ts::EASInbandDetailsChannelDescriptor::EASInbandDetailsChannelDescriptor(DuckContext& duck, const Descriptor& desc) :
    EASInbandDetailsChannelDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::EASInbandDetailsChannelDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(details_RF_channel);
    buf.putUInt16(details_program_number);
}

void ts::EASInbandDetailsChannelDescriptor::deserializePayload(PSIBuffer& buf)
{
    details_RF_channel= buf.getUInt8();
    details_program_number = buf.getUInt16();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::EASInbandDetailsChannelDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        disp << margin << UString::Format(u"RF channel: %d", {buf.getUInt8()});
        disp << UString::Format(u", program number: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::EASInbandDetailsChannelDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"details_RF_channel", details_RF_channel, false);
    root->setIntAttribute(u"details_program_number", details_program_number, true);
}

bool ts::EASInbandDetailsChannelDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(details_RF_channel, u"details_RF_channel", true) &&
           element->getIntAttribute(details_program_number, u"details_program_number", true);
}
