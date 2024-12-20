//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPMACGenericStreamLocationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"IPMAC_generic_stream_location_descriptor"
#define MY_CLASS    ts::IPMACGenericStreamLocationDescriptor
#define MY_EDID     ts::EDID::TableSpecific(ts::DID_INT_GEN_STREAM_LOC, ts::Standards::DVB, ts::TID_INT)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);

namespace {
    const ts::Enumeration ModulationTypeNames({
        {u"DVB-S2",  0},
        {u"DVB-T2",  1},
        {u"DVB-C2",  2},
        {u"DVB-NGH", 3},
    });
}

//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::IPMACGenericStreamLocationDescriptor::IPMACGenericStreamLocationDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::IPMACGenericStreamLocationDescriptor::IPMACGenericStreamLocationDescriptor(DuckContext& duck, const Descriptor& desc) :
    IPMACGenericStreamLocationDescriptor()
{
    deserialize(duck, desc);
}

void ts::IPMACGenericStreamLocationDescriptor::clearContent()
{
    interactive_network_id = 0;
    modulation_system_type = 0;
    modulation_system_id = 0;
    PHY_stream_id = 0;
    selector_bytes.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::IPMACGenericStreamLocationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(interactive_network_id);
    buf.putUInt8(modulation_system_type);
    buf.putUInt16(modulation_system_id);
    buf.putUInt16(PHY_stream_id);
    buf.putBytes(selector_bytes);
}

void ts::IPMACGenericStreamLocationDescriptor::deserializePayload(PSIBuffer& buf)
{
    interactive_network_id = buf.getUInt16();
    modulation_system_type = buf.getUInt8();
    modulation_system_id = buf.getUInt16();
    PHY_stream_id = buf.getUInt16();
    buf.getBytes(selector_bytes);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::IPMACGenericStreamLocationDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(7)) {
        disp << margin << UString::Format(u"Interactive network id: %n", buf.getUInt16()) << std::endl;
        const uint8_t systype = buf.getUInt8();
        disp << margin << UString::Format(u"Modulation system type: 0x%X (%s)", systype, ModulationTypeNames.name(systype)) << std::endl;
        disp << margin << UString::Format(u"Modulation system id: %n", buf.getUInt16()) << std::endl;
        disp << margin << UString::Format(u"Physical stream id: %n", buf.getUInt16()) << std::endl;
        disp.displayPrivateData(u"Selector bytes", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::IPMACGenericStreamLocationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"interactive_network_id", interactive_network_id, true);
    root->setEnumAttribute(ModulationTypeNames, u"modulation_system_type", modulation_system_type);
    root->setIntAttribute(u"modulation_system_id", modulation_system_id, true);
    root->setIntAttribute(u"PHY_stream_id", PHY_stream_id, true);
    root->addHexaTextChild(u"selector_bytes", selector_bytes, true);
}

bool ts::IPMACGenericStreamLocationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(interactive_network_id, u"interactive_network_id", true) &&
           element->getEnumAttribute(modulation_system_type, ModulationTypeNames, u"modulation_system_type", true) &&
           element->getIntAttribute(modulation_system_id, u"modulation_system_id", false) &&
           element->getIntAttribute(PHY_stream_id, u"PHY_stream_id", false) &&
           element->getHexaTextChild(selector_bytes, u"selector_bytes", false, 0, MAX_DESCRIPTOR_SIZE - 9);
}
