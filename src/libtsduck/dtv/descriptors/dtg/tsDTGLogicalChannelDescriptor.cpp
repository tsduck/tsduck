//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDTGLogicalChannelDescriptor.h"
#include "tsPSIRepository.h"

#define MY_XML_NAME u"dtg_logical_channel_descriptor"
#define MY_CLASS    ts::DTGLogicalChannelDescriptor
#define MY_EDID     ts::EDID::PrivateDVB(ts::DID_OFCOM_LOGICAL_CHAN, ts::PDS_OFCOM)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DTGLogicalChannelDescriptor::DTGLogicalChannelDescriptor() :
    AbstractLogicalChannelDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::DTGLogicalChannelDescriptor::DTGLogicalChannelDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractLogicalChannelDescriptor(duck, desc, MY_EDID, MY_XML_NAME)
{
}

ts::DTGLogicalChannelDescriptor::~DTGLogicalChannelDescriptor()
{
}
