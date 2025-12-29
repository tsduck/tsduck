//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsApplicationNameDescriptor.h"
#include "tsDescriptor.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"

#define MY_XML_NAME u"application_name_descriptor"
#define MY_XML_ATTR u"application_name"
#define MY_CLASS    ts::ApplicationNameDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_AIT_APP_NAME, ts::Standards::DVB, ts::TID_AIT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::ApplicationNameDescriptor::ApplicationNameDescriptor() :
    AbstractMultilingualDescriptor(MY_EDID, MY_XML_NAME, MY_XML_ATTR)
{
}

ts::ApplicationNameDescriptor::ApplicationNameDescriptor(DuckContext& duck, const Descriptor& desc) :
    ApplicationNameDescriptor()
{
    deserialize(duck, desc);
}

ts::ApplicationNameDescriptor::~ApplicationNameDescriptor()
{
}
