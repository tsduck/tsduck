//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEacemPreferredNameIdentifierDescriptor.h"
#include "tsPSIRepository.h"

#define MY_XML_NAME u"eacem_preferred_name_identifier_descriptor"
#define MY_CLASS    ts::EacemPreferredNameIdentifierDescriptor
#define MY_EDID     ts::EDID::PrivateDVB(ts::DID_EACEM_PREF_NAME_ID, ts::PDS_EACEM)
#define MY_EDID_1   ts::EDID::PrivateDVB(ts::DID_EACEM_PREF_NAME_ID, ts::PDS_TPS)

// Incorrect use of TPS private data, TPS broadcasters should use EACEM/EICTA PDS instead.
TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);
TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID_1, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::EacemPreferredNameIdentifierDescriptor::EacemPreferredNameIdentifierDescriptor(uint8_t id) :
    AbstractPreferredNameIdentifierDescriptor(id, MY_EDID, MY_XML_NAME)
{
}

ts::EacemPreferredNameIdentifierDescriptor::EacemPreferredNameIdentifierDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractPreferredNameIdentifierDescriptor(duck, desc, MY_EDID, MY_XML_NAME)
{
}

ts::EacemPreferredNameIdentifierDescriptor::~EacemPreferredNameIdentifierDescriptor()
{
}
