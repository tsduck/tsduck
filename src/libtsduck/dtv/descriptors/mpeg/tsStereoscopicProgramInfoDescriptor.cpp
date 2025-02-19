//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsStereoscopicProgramInfoDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"stereoscopic_program_info_descriptor"
#define MY_CLASS    ts::StereoscopicProgramInfoDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_MPEG_STEREO_PROG_INFO, ts::Standards::MPEG)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::StereoscopicProgramInfoDescriptor::StereoscopicProgramInfoDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::StereoscopicProgramInfoDescriptor::clearContent()
{
    stereoscopic_service_type = 0;
}

ts::StereoscopicProgramInfoDescriptor::StereoscopicProgramInfoDescriptor(DuckContext& duck, const Descriptor& desc) :
    StereoscopicProgramInfoDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::StereoscopicProgramInfoDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(0xFF, 5);
    buf.putBits(stereoscopic_service_type, 3);
}

void ts::StereoscopicProgramInfoDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.skipBits(5);
    buf.getBits(stereoscopic_service_type, 3);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::StereoscopicProgramInfoDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(1)) {
        buf.skipBits(5);
        disp << margin << "Stereoscopic service type: " << DataName(MY_XML_NAME, u"ServiceType", buf.getBits<uint8_t>(3), NamesFlags::DEC_VALUE_NAME) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::StereoscopicProgramInfoDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"stereoscopic_service_type", stereoscopic_service_type);
}

bool ts::StereoscopicProgramInfoDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(stereoscopic_service_type, u"stereoscopic_service_type", true, 0, 0, 7);
}
