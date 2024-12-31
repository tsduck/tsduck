//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEacemHDSimulcastLogicalChannelDescriptor.h"
#include "tsPSIRepository.h"

#define MY_XML_NAME u"eacem_HD_simulcast_logical_channel_descriptor"
#define MY_XML_NAME_LEGACY u"HD_simulcast_logical_channel_descriptor"
#define MY_CLASS    ts::EacemHDSimulcastLogicalChannelDescriptor
#define MY_EDID     ts::EDID::PrivateDVB(ts::DID_EACEM_HD_SIMULCAST_LCN, ts::PDS_EACEM)
#define MY_EDID_1   ts::EDID::PrivateDVB(ts::DID_EACEM_HD_SIMULCAST_LCN, ts::PDS_TPS)

// Incorrect use of TPS private data, TPS broadcasters should use EACEM/EICTA PDS instead.
TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor, MY_XML_NAME_LEGACY);
TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID_1, MY_XML_NAME, MY_CLASS::DisplayDescriptor, MY_XML_NAME_LEGACY);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::EacemHDSimulcastLogicalChannelDescriptor::EacemHDSimulcastLogicalChannelDescriptor() :
    AbstractLogicalChannelDescriptor(MY_EDID, MY_XML_NAME, MY_XML_NAME_LEGACY)
{
}

ts::EacemHDSimulcastLogicalChannelDescriptor::EacemHDSimulcastLogicalChannelDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractLogicalChannelDescriptor(duck, desc, MY_EDID, MY_XML_NAME, MY_XML_NAME_LEGACY)
{
}

ts::EacemHDSimulcastLogicalChannelDescriptor::~EacemHDSimulcastLogicalChannelDescriptor()
{
}
