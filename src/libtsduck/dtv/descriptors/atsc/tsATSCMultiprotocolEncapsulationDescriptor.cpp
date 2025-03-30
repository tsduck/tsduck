//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsATSCMultiprotocolEncapsulationDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ATSC_multiprotocol_encapsulation_descriptor"
#define MY_CLASS    ts::ATSCMultiprotocolEncapsulationDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ATSC_MPROTO_ENCAPS, ts::Standards::ATSC)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ATSCMultiprotocolEncapsulationDescriptor::ATSCMultiprotocolEncapsulationDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::ATSCMultiprotocolEncapsulationDescriptor::clearContent()
{
    deviceId_address_range = 6;
    deviceId_IP_mapping_flag = true;
    alignment_indicator = false;
    max_sections_per_datagram = 1;
}

ts::ATSCMultiprotocolEncapsulationDescriptor::ATSCMultiprotocolEncapsulationDescriptor(DuckContext& duck, const Descriptor& desc) :
    ATSCMultiprotocolEncapsulationDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization / deserialization
//----------------------------------------------------------------------------

void ts::ATSCMultiprotocolEncapsulationDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(deviceId_address_range, 3);
    buf.putBit(deviceId_IP_mapping_flag);
    buf.putBit(alignment_indicator);
    buf.putReserved(3);
    buf.putUInt8(max_sections_per_datagram);
}

void ts::ATSCMultiprotocolEncapsulationDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(deviceId_address_range, 3);
    deviceId_IP_mapping_flag = buf.getBool();
    alignment_indicator = buf.getBool();
    buf.skipReservedBits(3);
    max_sections_per_datagram = buf.getUInt8();
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ATSCMultiprotocolEncapsulationDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(2)) {
        disp << margin << "Device id address range: " << DataName(MY_XML_NAME, u"address_range", buf.getBits<uint8_t>(3), NamesFlags::DEC_VALUE_NAME) << std::endl;
        disp << margin << "Device id IP mapping: " << UString::YesNo(buf.getBool()) << std::endl;
        disp << margin << "Alignment indicator: " << UString::YesNo(buf.getBool()) << std::endl;
        buf.skipReservedBits(3);
        disp << margin << "Max sections per datagram: " << int(buf.getUInt8()) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML
//----------------------------------------------------------------------------

void ts::ATSCMultiprotocolEncapsulationDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"deviceId_address_range", deviceId_address_range);
    root->setBoolAttribute(u"deviceId_IP_mapping_flag", deviceId_IP_mapping_flag);
    root->setBoolAttribute(u"alignment_indicator", alignment_indicator);
    root->setIntAttribute(u"max_sections_per_datagram", max_sections_per_datagram);
}

bool ts::ATSCMultiprotocolEncapsulationDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(deviceId_address_range, u"deviceId_address_range", false, 6, 0, 7) &&
           element->getBoolAttribute(deviceId_IP_mapping_flag, u"deviceId_IP_mapping_flag", false, true) &&
           element->getBoolAttribute(alignment_indicator, u"alignment_indicator", false, false) &&
           element->getIntAttribute(max_sections_per_datagram, u"max_sections_per_datagram", false, 1);
}
