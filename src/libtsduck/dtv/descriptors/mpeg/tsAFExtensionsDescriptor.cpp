//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAFExtensionsDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"af_extensions_descriptor"
#define MY_CLASS ts::AFExtensionsDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_AF_EXT
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AFExtensionsDescriptor::AFExtensionsDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::AFExtensionsDescriptor::clearContent()
{
}

ts::AFExtensionsDescriptor::AFExtensionsDescriptor(DuckContext& duck, const Descriptor& desc) :
    AFExtensionsDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::AFExtensionsDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization / deserialization / display (empty extended payload).
//----------------------------------------------------------------------------

void ts::AFExtensionsDescriptor::serializePayload(PSIBuffer& buf) const
{
}

void ts::AFExtensionsDescriptor::deserializePayload(PSIBuffer& buf)
{
}

void ts::AFExtensionsDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
}


//----------------------------------------------------------------------------
// XML serialization / deserialization
//----------------------------------------------------------------------------

void ts::AFExtensionsDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
}

bool ts::AFExtensionsDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return true;
}
