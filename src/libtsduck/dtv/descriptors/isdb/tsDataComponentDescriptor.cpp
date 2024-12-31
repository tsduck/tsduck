//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDataComponentDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"data_component_descriptor"
#define MY_CLASS    ts::DataComponentDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ISDB_DATA_COMP, ts::Standards::ISDB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DataComponentDescriptor::DataComponentDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::DataComponentDescriptor::DataComponentDescriptor(DuckContext& duck, const Descriptor& desc) :
    DataComponentDescriptor()
{
    deserialize(duck, desc);
}

void ts::DataComponentDescriptor::clearContent()
{
    data_component_id = 0;
    additional_data_component_info.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DataComponentDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(data_component_id);
    buf.putBytes(additional_data_component_info);
}

void ts::DataComponentDescriptor::deserializePayload(PSIBuffer& buf)
{
    data_component_id = buf.getUInt16();
    buf.getBytes(additional_data_component_info);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DataComponentDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(2)) {
        disp << margin << "Data component id: " << DataName(MY_XML_NAME, u"DataComponentId", buf.getUInt16(), NamesFlags::HEXA_FIRST) << std::endl;
        disp.displayPrivateData(u"Additional data component info", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DataComponentDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"data_component_id", data_component_id, true);
    root->addHexaTextChild(u"additional_data_component_info", additional_data_component_info, true);
}

bool ts::DataComponentDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(data_component_id, u"data_component_id", true) &&
           element->getHexaTextChild(additional_data_component_info, u"additional_data_component_info", false, 0, MAX_DESCRIPTOR_SIZE - 2);
}
