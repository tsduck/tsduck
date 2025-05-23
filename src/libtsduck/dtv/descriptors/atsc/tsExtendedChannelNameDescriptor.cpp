//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsExtendedChannelNameDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"extended_channel_name_descriptor"
#define MY_CLASS    ts::ExtendedChannelNameDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ATSC_EXT_CHAN_NAME, ts::Standards::ATSC)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ExtendedChannelNameDescriptor::ExtendedChannelNameDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::ExtendedChannelNameDescriptor::clearContent()
{
    long_channel_name_text.clear();
}

ts::ExtendedChannelNameDescriptor::ExtendedChannelNameDescriptor(DuckContext& duck, const Descriptor& desc) :
    ExtendedChannelNameDescriptor()
{
    deserialize(duck, desc);
}

ts::DescriptorDuplication ts::ExtendedChannelNameDescriptor::duplicationMode() const
{
    return DescriptorDuplication::REPLACE;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ExtendedChannelNameDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putMultipleString(long_channel_name_text);
}

void ts::ExtendedChannelNameDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getMultipleString(long_channel_name_text);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ExtendedChannelNameDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    disp.displayATSCMultipleString(buf, 0, margin, u"Long channel name: ");
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ExtendedChannelNameDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    long_channel_name_text.toXML(duck, root, u"long_channel_name_text", true);
}

bool ts::ExtendedChannelNameDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return long_channel_name_text.fromXML(duck, element, u"long_channel_name_text", false);
}
