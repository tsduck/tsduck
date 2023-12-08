//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMultilingualNetworkNameDescriptor.h"
#include "tsDescriptor.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"

#define MY_XML_NAME u"multilingual_network_name_descriptor"
#define MY_XML_ATTR u"network_name"
#define MY_CLASS ts::MultilingualNetworkNameDescriptor
#define MY_DID ts::DID_MLINGUAL_NETWORK

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::MultilingualNetworkNameDescriptor::MultilingualNetworkNameDescriptor() :
    AbstractMultilingualDescriptor(MY_DID, MY_XML_NAME, MY_XML_ATTR)
{
}

ts::MultilingualNetworkNameDescriptor::MultilingualNetworkNameDescriptor(DuckContext& duck, const Descriptor& desc) :
    MultilingualNetworkNameDescriptor()
{
    deserialize(duck, desc);
}

ts::MultilingualNetworkNameDescriptor::~MultilingualNetworkNameDescriptor()
{
}
