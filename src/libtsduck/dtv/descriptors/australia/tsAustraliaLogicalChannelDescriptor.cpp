//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAustraliaLogicalChannelDescriptor.h"
#include "tsPSIRepository.h"

#define MY_XML_NAME u"australia_logical_channel_descriptor"
#define MY_CLASS    ts::AustraliaLogicalChannelDescriptor
#define MY_EDID(n)  ts::EDID::PrivateDVB(ts::DID_AUSTRALIA_LOGICAL_CHAN, ts::PDS_AUSTRALIA + (n))

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID(0), MY_XML_NAME, MY_CLASS::DisplayDescriptor);

// Incorrect use of Free TV Australia private data for broadcasters.
TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID(1), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID(2), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID(3), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID(4), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID(5), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID(10), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID(11), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID(12), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID(13), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID(14), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID(15), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AustraliaLogicalChannelDescriptor::AustraliaLogicalChannelDescriptor() :
    AbstractLogicalChannelDescriptor(MY_EDID(0), MY_XML_NAME)
{
}

ts::AustraliaLogicalChannelDescriptor::AustraliaLogicalChannelDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractLogicalChannelDescriptor(duck, desc, MY_EDID(0), MY_XML_NAME)
{
}

ts::AustraliaLogicalChannelDescriptor::~AustraliaLogicalChannelDescriptor()
{
}
