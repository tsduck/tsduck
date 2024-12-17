//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2024, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCCompressedModuleDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"dsmcc_compressed_module_descriptor"
#define MY_CLASS    ts::DSMCCCompressedModuleDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_DSMCC_COMPRESSED_MODULE, ts::Standards::DVB, ts::TID_DSMCC_UNM)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::DSMCCCompressedModuleDescriptor::DSMCCCompressedModuleDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::DSMCCCompressedModuleDescriptor::DSMCCCompressedModuleDescriptor(DuckContext& duck, const Descriptor& desc) :
    DSMCCCompressedModuleDescriptor()
{
    deserialize(duck, desc);
}

void ts::DSMCCCompressedModuleDescriptor::clearContent()
{
    compression_method = 0;
    original_size = 0;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DSMCCCompressedModuleDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(5)) {
        const uint8_t  compression_method = buf.getUInt8();
        const uint32_t original_size = buf.getUInt32();
        disp << margin << UString::Format(u"Compression Method: %n", compression_method) << std::endl;
        disp << margin << UString::Format(u"Original Size: %n", original_size) << std::endl;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DSMCCCompressedModuleDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(compression_method);
    buf.putUInt32(original_size);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DSMCCCompressedModuleDescriptor::deserializePayload(PSIBuffer& buf)
{
    compression_method = buf.getUInt8();
    original_size = buf.getUInt32();
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DSMCCCompressedModuleDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"compression_method", compression_method, true);
    root->setIntAttribute(u"original_size", original_size, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DSMCCCompressedModuleDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(compression_method, u"compression_method", true) &&
           element->getIntAttribute(original_size, u"original_size", true);
}
