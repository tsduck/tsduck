//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsVBITeletextDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"

#define MY_XML_NAME u"VBI_teletext_descriptor"
#define MY_CLASS ts::VBITeletextDescriptor
#define MY_DID ts::DID_VBI_TELETEXT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::VBITeletextDescriptor::VBITeletextDescriptor() :
    TeletextDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::VBITeletextDescriptor::VBITeletextDescriptor(DuckContext& duck, const Descriptor& desc) :
    VBITeletextDescriptor()
{
    deserialize(duck, desc);
}

ts::VBITeletextDescriptor::~VBITeletextDescriptor()
{
}
