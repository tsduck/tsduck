//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDTGPreferredNameIdentifierDescriptor.h"
#include "tsPSIRepository.h"

#define MY_XML_NAME u"dtg_preferred_name_identifier_descriptor"
#define MY_CLASS    ts::DTGPreferredNameIdentifierDescriptor
#define MY_EDID     ts::EDID::PrivateDVB(ts::DID_OFCOM_PREF_NAME_ID, ts::PDS_OFCOM)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DTGPreferredNameIdentifierDescriptor::DTGPreferredNameIdentifierDescriptor(uint8_t id) :
    AbstractPreferredNameIdentifierDescriptor(id, MY_EDID, MY_XML_NAME)
{
}

ts::DTGPreferredNameIdentifierDescriptor::DTGPreferredNameIdentifierDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractPreferredNameIdentifierDescriptor(duck, desc, MY_EDID, MY_XML_NAME)
{
}

ts::DTGPreferredNameIdentifierDescriptor::~DTGPreferredNameIdentifierDescriptor()
{
}
