//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDTGPreferredNameListDescriptor.h"
#include "tsPSIRepository.h"

#define MY_XML_NAME u"dtg_preferred_name_list_descriptor"
#define MY_CLASS ts::DTGPreferredNameListDescriptor
#define MY_DID ts::DID_OFCOM_PREF_NAME_LST
#define MY_PDS ts::PDS_OFCOM
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DTGPreferredNameListDescriptor::DTGPreferredNameListDescriptor() :
    AbstractPreferredNameListDescriptor(MY_DID, MY_XML_NAME, MY_STD, MY_PDS)
{
}

ts::DTGPreferredNameListDescriptor::DTGPreferredNameListDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractPreferredNameListDescriptor(duck, desc, MY_DID, MY_XML_NAME, MY_STD, MY_PDS)
{
}

ts::DTGPreferredNameListDescriptor::~DTGPreferredNameListDescriptor()
{
}
