//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsC2DeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"C2_delivery_system_descriptor"
#define MY_CLASS ts::C2DeliverySystemDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_C2_DELIVERY

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::C2DeliverySystemDescriptor::C2DeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(MY_DID, DS_DVB_C2, MY_XML_NAME)
{
}

void ts::C2DeliverySystemDescriptor::clearContent()
{
    plp_id = 0;
    data_slice_id = 0;
    C2_system_tuning_frequency = 0;
    C2_system_tuning_frequency_type = 0;
    active_OFDM_symbol_duration = 0;
    guard_interval = 0;
}

ts::C2DeliverySystemDescriptor::C2DeliverySystemDescriptor(DuckContext& duck, const Descriptor& desc) :
    C2DeliverySystemDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::C2DeliverySystemDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::C2DeliverySystemDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(plp_id);
    buf.putUInt8(data_slice_id);
    buf.putUInt32(C2_system_tuning_frequency);
    buf.putBits(C2_system_tuning_frequency_type, 2);
    buf.putBits(active_OFDM_symbol_duration, 3);
    buf.putBits(guard_interval, 3);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::C2DeliverySystemDescriptor::deserializePayload(PSIBuffer& buf)
{
    plp_id = buf.getUInt8();
    data_slice_id = buf.getUInt8();
    C2_system_tuning_frequency = buf.getUInt32();
    buf.getBits(C2_system_tuning_frequency_type, 2);
    buf.getBits(active_OFDM_symbol_duration, 3);
    buf.getBits(guard_interval, 3);
}


//----------------------------------------------------------------------------
// Enumerations for XML.
//----------------------------------------------------------------------------

const ts::Enumeration ts::C2DeliverySystemDescriptor::C2GuardIntervalNames({
    {u"1/128", 0},
    {u"1/64",  1},
});


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::C2DeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(7)) {
        disp << margin << UString::Format(u"PLP id: 0x%X (%<d)", {buf.getUInt8()});
        disp << UString::Format(u", data slice id: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        disp << margin << UString::Format(u"Frequency: %'d Hz (0x%<X)", {buf.getUInt32()}) << std::endl;
        disp << margin << UString::Format(u"Tuning frequency type: %s", {DataName(MY_XML_NAME, u"C2TuningType", buf.getBits<uint8_t>(2), NamesFlags::FIRST)}) << std::endl;
        disp << margin << UString::Format(u"Symbol duration: %s", {DataName(MY_XML_NAME, u"C2SymbolDuration", buf.getBits<uint8_t>(3), NamesFlags::FIRST)}) << std::endl;
        const uint8_t guard = buf.getBits<uint8_t>(3);
        disp << margin << UString::Format(u"Guard interval: %d (%s)", {guard, C2GuardIntervalNames.name(guard)}) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::C2DeliverySystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"plp_id", plp_id, true);
    root->setIntAttribute(u"data_slice_id", data_slice_id, true);
    root->setIntAttribute(u"C2_system_tuning_frequency", C2_system_tuning_frequency);
    root->setIntAttribute(u"C2_system_tuning_frequency_type", C2_system_tuning_frequency_type);
    root->setIntAttribute(u"active_OFDM_symbol_duration", active_OFDM_symbol_duration);
    root->setIntEnumAttribute(C2GuardIntervalNames, u"guard_interval", guard_interval);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::C2DeliverySystemDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return  element->getIntAttribute(plp_id, u"plp_id", true) &&
            element->getIntAttribute(data_slice_id, u"data_slice_id", true) &&
            element->getIntAttribute(C2_system_tuning_frequency, u"C2_system_tuning_frequency", true) &&
            element->getIntAttribute(C2_system_tuning_frequency_type, u"C2_system_tuning_frequency_type", true, 0, 0, 3) &&
            element->getIntAttribute(active_OFDM_symbol_duration, u"active_OFDM_symbol_duration", true, 0, 0, 7) &&
            element->getIntEnumAttribute(guard_interval, C2GuardIntervalNames, u"guard_interval", true);
}
