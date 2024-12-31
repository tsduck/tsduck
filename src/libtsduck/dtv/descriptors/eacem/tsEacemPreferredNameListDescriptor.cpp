//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEacemPreferredNameListDescriptor.h"
#include "tsPSIRepository.h"

#define MY_XML_NAME u"eacem_preferred_name_list_descriptor"
#define MY_CLASS    ts::EacemPreferredNameListDescriptor
#define MY_EDID     ts::EDID::PrivateDVB(ts::DID_EACEM_PREF_NAME_LIST, ts::PDS_EACEM)
#define MY_EDID_1   ts::EDID::PrivateDVB(ts::DID_EACEM_PREF_NAME_LIST, ts::PDS_TPS)

// Incorrect use of TPS private data, TPS broadcasters should use EACEM/EICTA PDS instead.
TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID_1, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::EacemPreferredNameListDescriptor::EacemPreferredNameListDescriptor() :
    AbstractPreferredNameListDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::EacemPreferredNameListDescriptor::EacemPreferredNameListDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractPreferredNameListDescriptor(duck, desc, MY_EDID, MY_XML_NAME)
{
}

ts::EacemPreferredNameListDescriptor::~EacemPreferredNameListDescriptor()
{
}
