//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsATSCModuleLinkDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ATSC_module_link_descriptor"
#define MY_CLASS    ts::ATSCModuleLinkDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ATSC_MODULE_LINK, ts::Standards::ATSC)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ATSCModuleLinkDescriptor::ATSCModuleLinkDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::ATSCModuleLinkDescriptor::clearContent()
{
    position = 0;
    module_id = 0;
}

ts::ATSCModuleLinkDescriptor::ATSCModuleLinkDescriptor(DuckContext& duck, const Descriptor& desc) :
    ATSCModuleLinkDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::ATSCModuleLinkDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(position);
    buf.putUInt16(module_id);
}

void ts::ATSCModuleLinkDescriptor::deserializePayload(PSIBuffer& buf)
{
    position = buf.getUInt8();
    module_id = buf.getUInt16();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ATSCModuleLinkDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(3)) {
        disp << margin << "Position: " << DataName(MY_XML_NAME, u"position", buf.getUInt8(), NamesFlags::HEX_VALUE_NAME) << std::endl;
        disp << margin << UString::Format(u"Module id: %n", buf.getUInt16()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML
//----------------------------------------------------------------------------

void ts::ATSCModuleLinkDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"position", position);
    root->setIntAttribute(u"module_id", module_id, true);
}

bool ts::ATSCModuleLinkDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(position, u"position", true) &&
           element->getIntAttribute(module_id, u"module_id", true);
}
