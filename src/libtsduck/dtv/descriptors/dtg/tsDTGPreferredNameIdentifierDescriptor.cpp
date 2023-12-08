//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDTGPreferredNameIdentifierDescriptor.h"
#include "tsPSIRepository.h"

#define MY_XML_NAME u"dtg_preferred_name_identifier_descriptor"
#define MY_CLASS ts::DTGPreferredNameIdentifierDescriptor
#define MY_DID ts::DID_OFCOM_PREF_NAME_ID
#define MY_PDS ts::PDS_OFCOM
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DTGPreferredNameIdentifierDescriptor::DTGPreferredNameIdentifierDescriptor(uint8_t id) :
    AbstractPreferredNameIdentifierDescriptor(id, MY_DID, MY_XML_NAME, MY_STD, MY_PDS)
{
}

ts::DTGPreferredNameIdentifierDescriptor::DTGPreferredNameIdentifierDescriptor(DuckContext& duck, const Descriptor& desc) :
    AbstractPreferredNameIdentifierDescriptor(duck, desc, MY_DID, MY_XML_NAME, MY_STD, MY_PDS)
{
}

ts::DTGPreferredNameIdentifierDescriptor::~DTGPreferredNameIdentifierDescriptor()
{
}
