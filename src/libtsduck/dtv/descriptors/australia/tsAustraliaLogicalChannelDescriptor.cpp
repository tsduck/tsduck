//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAustraliaLogicalChannelDescriptor.h"
#include "tsPSIRepository.h"

#define MY_XML_NAME u"australia_logical_channel_descriptor"
#define MY_CLASS ts::AustraliaLogicalChannelDescriptor
#define MY_DID ts::DID_AUSTRALIA_LOGICAL_CHAN
#define MY_PDS ts::PDS_AUSTRALIA
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);

// Incorrect use of Free TV Australia private data for broadcasters.
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS + 1), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS + 2), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS + 3), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS + 4), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS + 5), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS + 10), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS + 11), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS + 12), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS + 13), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS + 14), MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS + 15), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AustraliaLogicalChannelDescriptor::AustraliaLogicalChannelDescriptor() :
    AbstractLogicalChannelDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS)
{
}

ts::AustraliaLogicalChannelDescriptor::AustraliaLogicalChannelDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractLogicalChannelDescriptor(duck, desc, MY_DID, MY_XML_NAME, MY_STD, MY_PDS)
{
}

ts::AustraliaLogicalChannelDescriptor::~AustraliaLogicalChannelDescriptor()
{
}
