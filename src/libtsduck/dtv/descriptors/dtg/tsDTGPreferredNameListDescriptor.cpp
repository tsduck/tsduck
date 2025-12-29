//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDTGPreferredNameListDescriptor.h"
#include "tsPSIRepository.h"

#define MY_XML_NAME u"dtg_preferred_name_list_descriptor"
#define MY_CLASS    ts::DTGPreferredNameListDescriptor
#define MY_EDID     ts::EDID::PrivateDVB(ts::DID_OFCOM_PREF_NAME_LST, ts::PDS_OFCOM)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DTGPreferredNameListDescriptor::DTGPreferredNameListDescriptor() :
    AbstractPreferredNameListDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::DTGPreferredNameListDescriptor::DTGPreferredNameListDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractPreferredNameListDescriptor(duck, desc, MY_EDID, MY_XML_NAME)
{
}

ts::DTGPreferredNameListDescriptor::~DTGPreferredNameListDescriptor()
{
}
