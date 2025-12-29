//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMPEGH3DAudioConfigDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"MPEGH_3D_audio_config_descriptor"
#define MY_CLASS    ts::MPEGH3DAudioConfigDescriptor
#define MY_EDID     ts::EDID::ExtensionMPEG(ts::XDID_MPEG_MPH3D_CONFIG)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MPEGH3DAudioConfigDescriptor::MPEGH3DAudioConfigDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::MPEGH3DAudioConfigDescriptor::clearContent()
{
    mpegh3daConfig.clear();
}

ts::MPEGH3DAudioConfigDescriptor::MPEGH3DAudioConfigDescriptor(DuckContext& duck, const Descriptor& desc) :
    MPEGH3DAudioConfigDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioConfigDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBytes(mpegh3daConfig);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioConfigDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBytes(mpegh3daConfig);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioConfigDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    disp.displayPrivateData(u"mpegh3daConfig", buf, NPOS, margin);
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioConfigDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->addHexaTextChild(u"mpegh3daConfig", mpegh3daConfig, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MPEGH3DAudioConfigDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getHexaTextChild(mpegh3daConfig, u"mpegh3daConfig", false, 0, MAX_DESCRIPTOR_SIZE);
}
