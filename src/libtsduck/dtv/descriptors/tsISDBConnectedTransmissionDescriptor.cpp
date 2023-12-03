//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISDBConnectedTransmissionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsNames.h"

#define MY_XML_NAME u"ISDB_connected_transmission_descriptor"
#define MY_CLASS    ts::ISDBConnectedTransmissionDescriptor
#define MY_DID      ts::DID_ISDB_CONNECT_TRANSM
#define MY_PDS      ts::PDS_ISDB
#define MY_STD      ts::Standards::ISDB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Private(MY_DID, MY_PDS), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ISDBConnectedTransmissionDescriptor::ISDBConnectedTransmissionDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::ISDBConnectedTransmissionDescriptor::ISDBConnectedTransmissionDescriptor(DuckContext& duck, const Descriptor& desc) :
    ISDBConnectedTransmissionDescriptor()
{
    deserialize(duck, desc);
}


void ts::ISDBConnectedTransmissionDescriptor::clearContent()
{
    connected_transmission_group_id = 0;
    segment_type = 0;
    modulation_type_A = 0;
    modulation_type_B = 0;
    modulation_type_C = 0;
    addtional_connected_transmission_info.clear();
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ISDBConnectedTransmissionDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(connected_transmission_group_id);
    buf.putBits(segment_type, 2);
    buf.putBits(modulation_type_A, 2);
    buf.putBits(modulation_type_B, 2);
    buf.putBits(modulation_type_C, 2);
    buf.putBytes(addtional_connected_transmission_info);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ISDBConnectedTransmissionDescriptor::deserializePayload(PSIBuffer& buf)
{
    connected_transmission_group_id = buf.getUInt16();
    segment_type = buf.getBits<uint8_t>(2);
    modulation_type_A = buf.getBits<uint8_t>(2);
    modulation_type_B = buf.getBits<uint8_t>(2);
    modulation_type_C = buf.getBits<uint8_t>(2);
    buf.getBytes(addtional_connected_transmission_info);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ISDBConnectedTransmissionDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(3)) {
        disp << margin << UString::Format(u"Connected transmission group id: 0x%X (%<d)", {buf.getUInt16()}) << std::endl;
        disp << margin << "Segment type: " << DataName(MY_XML_NAME, u"segment_type", buf.getBits<uint8_t>(2), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
        disp << margin << "Modulation type A: " << DataName(MY_XML_NAME, u"modulation_type", buf.getBits<uint8_t>(2), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
        disp << margin << "Modulation type B: " << DataName(MY_XML_NAME, u"modulation_type", buf.getBits<uint8_t>(2), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
        disp << margin << "Modulation type C: " << DataName(MY_XML_NAME, u"modulation_type", buf.getBits<uint8_t>(2), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;

        disp.displayPrivateData(u"Additional connected transmission info", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ISDBConnectedTransmissionDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"connected_transmission_group_id", connected_transmission_group_id, true);
    root->setIntAttribute(u"segment_type", segment_type, true);
    root->setIntAttribute(u"modulation_type_A", modulation_type_A, true);
    root->setIntAttribute(u"modulation_type_B", modulation_type_B, true);
    root->setIntAttribute(u"modulation_type_C", modulation_type_C, true);
    root->addHexaTextChild(u"addtional_connected_transmission_info", addtional_connected_transmission_info, true);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ISDBConnectedTransmissionDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(connected_transmission_group_id, u"connected_transmission_group_id", true) &&
           element->getIntAttribute(segment_type, u"segment_type", true) &&
           element->getIntAttribute(modulation_type_A, u"modulation_type_A", true) &&
           element->getIntAttribute(modulation_type_B, u"modulation_type_B", true) &&
           element->getIntAttribute(modulation_type_C, u"modulation_type_C", true) &&
           element->getHexaTextChild(addtional_connected_transmission_info, u"addtional_connected_transmission_info", false);
}
