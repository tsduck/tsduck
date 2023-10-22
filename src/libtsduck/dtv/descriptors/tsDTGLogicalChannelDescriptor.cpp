//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDTGLogicalChannelDescriptor.h"
#include "tsPSIRepository.h"

#define MY_XML_NAME u"dtg_logical_channel_descriptor"
#define MY_CLASS ts::DTGLogicalChannelDescriptor
#define MY_DID ts::DID_OFCOM_LOGICAL_CHAN
#define MY_PDS ts::PDS_OFCOM
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DTGLogicalChannelDescriptor::DTGLogicalChannelDescriptor() :
    AbstractLogicalChannelDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS)
{
}

ts::DTGLogicalChannelDescriptor::DTGLogicalChannelDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractLogicalChannelDescriptor(duck, desc, MY_DID, MY_XML_NAME, MY_STD, MY_PDS)
{
}

ts::DTGLogicalChannelDescriptor::~DTGLogicalChannelDescriptor()
{
}
