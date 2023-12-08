//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEacemLogicalChannelNumberDescriptor.h"
#include "tsPSIRepository.h"

#define MY_XML_NAME u"eacem_logical_channel_number_descriptor"
#define MY_XML_NAME_LEGACY u"logical_channel_number_descriptor"
#define MY_CLASS ts::EacemLogicalChannelNumberDescriptor
#define MY_DID ts::DID_LOGICAL_CHANNEL_NUM
#define MY_PDS ts::PDS_EACEM
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor, MY_XML_NAME_LEGACY);

// Incorrect use of TPS private data, TPS broadcasters should use EACEM/EICTA PDS instead.
TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, ts::PDS_TPS), MY_XML_NAME, MY_CLASS::DisplayDescriptor, MY_XML_NAME_LEGACY);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::EacemLogicalChannelNumberDescriptor::EacemLogicalChannelNumberDescriptor() :
    AbstractLogicalChannelDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS, MY_XML_NAME_LEGACY)
{
}

ts::EacemLogicalChannelNumberDescriptor::EacemLogicalChannelNumberDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractLogicalChannelDescriptor(duck, desc, MY_DID, MY_XML_NAME, MY_STD, MY_PDS, MY_XML_NAME_LEGACY)
{
}

ts::EacemLogicalChannelNumberDescriptor::~EacemLogicalChannelNumberDescriptor()
{
}
