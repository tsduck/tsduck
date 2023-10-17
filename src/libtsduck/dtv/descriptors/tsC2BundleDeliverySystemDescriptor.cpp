//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsC2BundleDeliverySystemDescriptor.h"
#include "tsC2DeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"C2_bundle_delivery_system_descriptor"
#define MY_CLASS ts::C2BundleDeliverySystemDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_C2_BUNDLE_DELIVERY

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors.
//----------------------------------------------------------------------------

ts::C2BundleDeliverySystemDescriptor::C2BundleDeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(MY_DID, DS_DVB_C2, MY_XML_NAME)
{
}

ts::C2BundleDeliverySystemDescriptor::C2BundleDeliverySystemDescriptor(DuckContext& duck, const Descriptor& desc) :
    C2BundleDeliverySystemDescriptor()
{
    deserialize(duck, desc);
}

void ts::C2BundleDeliverySystemDescriptor::clearContent()
{

    entries.clear();
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::C2BundleDeliverySystemDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::C2BundleDeliverySystemDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (const auto& it : entries) {
        buf.putUInt8(it.plp_id);
        buf.putUInt8(it.data_slice_id);
        buf.putUInt32(it.C2_system_tuning_frequency);
        buf.putBits(it.C2_system_tuning_frequency_type, 2);
        buf.putBits(it.active_OFDM_symbol_duration, 3);
        buf.putBits(it.guard_interval, 3);
        buf.putBit(it.master_channel);
        buf.putBits(0x00, 7); // reserved_zero_future_use
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::C2BundleDeliverySystemDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canRead()) {
        Entry e;
        e.plp_id = buf.getUInt8();
        e.data_slice_id = buf.getUInt8();
        e.C2_system_tuning_frequency = buf.getUInt32();
        buf.getBits(e.C2_system_tuning_frequency_type, 2);
        buf.getBits(e.active_OFDM_symbol_duration, 3);
        buf.getBits(e.guard_interval, 3);
        e.master_channel = buf.getBool();
        buf.skipBits(7);
        entries.push_back(e);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::C2BundleDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    while (buf.canReadBytes(8)) {
        disp << margin << UString::Format(u"- PLP id: 0x%X (%<d)", {buf.getUInt8()});
        disp << UString::Format(u", data slice id: 0x%X (%<d)", {buf.getUInt8()}) << std::endl;
        disp << margin << UString::Format(u"  Frequency: %'d Hz (0x%<X)", {buf.getUInt32()}) << std::endl;
        disp << margin << UString::Format(u"  Tuning frequency type: %s", {DataName(MY_XML_NAME, u"C2TuningType", buf.getBits<uint8_t>(2), NamesFlags::FIRST)}) << std::endl;
        disp << margin << UString::Format(u"  Symbol duration: %s", {DataName(MY_XML_NAME, u"C2SymbolDuration", buf.getBits<uint8_t>(3), NamesFlags::FIRST)}) << std::endl;
        const uint8_t guard = buf.getBits<uint8_t>(3);
        disp << margin << UString::Format(u"  Guard interval: %d (%s)", {guard, C2DeliverySystemDescriptor::C2GuardIntervalNames.name(guard)}) << std::endl;
        disp << margin << UString::Format(u"  Master channel: %s", {buf.getBool()}) << std::endl;
        buf.skipBits(7);
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::C2BundleDeliverySystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (const auto& it : entries) {
        xml::Element* e = root->addElement(u"plp");
        e->setIntAttribute(u"plp_id", it.plp_id, true);
        e->setIntAttribute(u"data_slice_id", it.data_slice_id, true);
        e->setIntAttribute(u"C2_system_tuning_frequency", it.C2_system_tuning_frequency);
        e->setIntAttribute(u"C2_system_tuning_frequency_type", it.C2_system_tuning_frequency_type);
        e->setIntAttribute(u"active_OFDM_symbol_duration", it.active_OFDM_symbol_duration);
        e->setIntEnumAttribute(C2DeliverySystemDescriptor::C2GuardIntervalNames, u"guard_interval", it.guard_interval);
        e->setBoolAttribute(u"master_channel", it.master_channel);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::C2BundleDeliverySystemDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"plp", 0, MAX_ENTRIES);

    for (size_t i = 0; ok && i < children.size(); ++i) {
        Entry e;
        ok = children[i]->getIntAttribute(e.plp_id, u"plp_id", true) &&
             children[i]->getIntAttribute(e.data_slice_id, u"data_slice_id", true) &&
             children[i]->getIntAttribute(e.C2_system_tuning_frequency, u"C2_system_tuning_frequency", true) &&
             children[i]->getIntAttribute(e.C2_system_tuning_frequency_type, u"C2_system_tuning_frequency_type", true, 0, 0, 3) &&
             children[i]->getIntAttribute(e.active_OFDM_symbol_duration, u"active_OFDM_symbol_duration", true, 0, 0, 7) &&
             children[i]->getIntEnumAttribute(e.guard_interval, C2DeliverySystemDescriptor::C2GuardIntervalNames, u"guard_interval", true) &&
             children[i]->getBoolAttribute(e.master_channel, u"master_channel", true);
        entries.push_back(e);
    }
    return ok;
}
