//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsATSCCRC32Descriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ATSC_CRC32_descriptor"
#define MY_CLASS    ts::ATSCCRC32Descriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ATSC_CRC32, ts::Standards::ATSC)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ATSCCRC32Descriptor::ATSCCRC32Descriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::ATSCCRC32Descriptor::clearContent()
{
    CRC_32 = 0;
}

ts::ATSCCRC32Descriptor::ATSCCRC32Descriptor(DuckContext& duck, const Descriptor& desc) :
    ATSCCRC32Descriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::ATSCCRC32Descriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt32(CRC_32);
}

void ts::ATSCCRC32Descriptor::deserializePayload(PSIBuffer& buf)
{
    CRC_32 = buf.getUInt32();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ATSCCRC32Descriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(4)) {
        disp << margin << UString::Format(u"CRC-32: 0x%X", buf.getUInt32()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML
//----------------------------------------------------------------------------

void ts::ATSCCRC32Descriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"CRC_32", CRC_32, true);
}

bool ts::ATSCCRC32Descriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(CRC_32, u"CRC_32", true);
}
