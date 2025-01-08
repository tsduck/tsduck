//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCCRC32Descriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"dsmcc_CRC32_descriptor"
#define MY_CLASS    ts::DSMCCCRC32Descriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_DSMCC_CRC32, ts::Standards::DVB, ts::TID_DSMCC_UNM)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::DSMCCCRC32Descriptor::DSMCCCRC32Descriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::DSMCCCRC32Descriptor::DSMCCCRC32Descriptor(DuckContext& duck, const Descriptor& desc) :
    DSMCCCRC32Descriptor()
{
    deserialize(duck, desc);
}

void ts::DSMCCCRC32Descriptor::clearContent()
{
    crc32 = 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DSMCCCRC32Descriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(4)) {
        const uint32_t crc32 = buf.getUInt32();
        disp << margin << UString::Format(u"CRC32: %n", crc32) << std::endl;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DSMCCCRC32Descriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt32(crc32);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCCRC32Descriptor::deserializePayload(PSIBuffer& buf)
{
    crc32 = buf.getUInt32();
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCCRC32Descriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"CRC_32", crc32, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DSMCCCRC32Descriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(crc32, u"CRC_32", true);
}
