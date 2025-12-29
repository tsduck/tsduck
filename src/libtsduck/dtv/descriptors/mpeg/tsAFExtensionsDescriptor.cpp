//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
#define MY_CLASS    ts::AFExtensionsDescriptor
#define MY_EDID     ts::EDID::ExtensionMPEG(ts::XDID_MPEG_AF_EXT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AFExtensionsDescriptor::AFExtensionsDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
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
// Serialization / deserialization / display (empty extended payload).
//----------------------------------------------------------------------------

void ts::AFExtensionsDescriptor::serializePayload(PSIBuffer& buf) const
{
}

void ts::AFExtensionsDescriptor::deserializePayload(PSIBuffer& buf)
{
}

void ts::AFExtensionsDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
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
