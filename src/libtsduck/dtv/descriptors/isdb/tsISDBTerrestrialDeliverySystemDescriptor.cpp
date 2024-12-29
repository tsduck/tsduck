//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsISDBTerrestrialDeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"ISDB_terrestrial_delivery_system_descriptor"
#define MY_CLASS    ts::ISDBTerrestrialDeliverySystemDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_ISDB_TERRES_DELIV, ts::Standards::ISDB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::ISDBTerrestrialDeliverySystemDescriptor::ISDBTerrestrialDeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(MY_EDID, DS_ISDB_T, MY_XML_NAME)
{
}

void ts::ISDBTerrestrialDeliverySystemDescriptor::clearContent()
{
    area_code = 0;
    guard_interval = 0;
    transmission_mode = 0;
    frequencies.clear();
}

ts::ISDBTerrestrialDeliverySystemDescriptor::ISDBTerrestrialDeliverySystemDescriptor(DuckContext& duck, const Descriptor& desc) :
    ISDBTerrestrialDeliverySystemDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Thread-safe init-safe static data patterns.
//----------------------------------------------------------------------------

const std::map<int, ts::TransmissionMode>& ts::ISDBTerrestrialDeliverySystemDescriptor::ToTransmissionMode()
{
    static const std::map<int, TransmissionMode> data {
        {0, TM_2K}, // Mode 1
        {1, TM_4K}, // Mode 2
        {2, TM_8K}, // Mode 3
    };
    return data;
}

const std::map<int, ts::GuardInterval>& ts::ISDBTerrestrialDeliverySystemDescriptor::ToGuardInterval()
{
    static const std::map<int, GuardInterval> data {
        {0, GUARD_1_32},
        {1, GUARD_1_16},
        {2, GUARD_1_8},
        {3, GUARD_1_4},
    };
    return data;
}

const ts::Enumeration& ts::ISDBTerrestrialDeliverySystemDescriptor::GuardIntervalNames()
{
    static const Enumeration data({
        {u"1/32", 0},
        {u"1/16", 1},
        {u"1/8",  2},
        {u"1/4",  3},
    });
    return data;
}

const ts::Enumeration& ts::ISDBTerrestrialDeliverySystemDescriptor::TransmissionModeNames()
{
    static const Enumeration data({
        {u"2k",        0},
        {u"mode1",     0},
        {u"4k",        1},
        {u"mode2",     1},
        {u"8k",        2},
        {u"mode3",     2},
        {u"undefined", 3},
    });
    return data;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ISDBTerrestrialDeliverySystemDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(area_code, 12);
    buf.putBits(guard_interval, 2);
    buf.putBits(transmission_mode, 2);
    for (auto it : frequencies) {
        buf.putUInt16(HzToBin(it));
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ISDBTerrestrialDeliverySystemDescriptor::deserializePayload(PSIBuffer& buf)
{
    buf.getBits(area_code, 12);
    buf.getBits(guard_interval, 2);
    buf.getBits(transmission_mode, 2);
    while (buf.canRead()) {
        frequencies.push_back(BinToHz(buf.getUInt16()));
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::ISDBTerrestrialDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"Area code: 0x%3X (%<d)", buf.getBits<uint16_t>(12)) << std::endl;
        const uint8_t guard = buf.getBits<uint8_t>(2);
        const uint8_t mode = buf.getBits<uint8_t>(2);
        disp << margin << UString::Format(u"Guard interval: %d (%s)", guard, GuardIntervalNames().name(guard)) << std::endl;
        disp << margin << UString::Format(u"Transmission mode: %d (%s)", mode, TransmissionModeNames().name(mode)) << std::endl;
    }
    while (buf.canReadBytes(2)) {
        disp << margin << UString::Format(u"Frequency: %'d Hz", BinToHz(buf.getUInt16())) << std::endl;
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::ISDBTerrestrialDeliverySystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"area_code", area_code, true);
    root->setEnumAttribute(GuardIntervalNames(), u"guard_interval", guard_interval);
    root->setEnumAttribute(TransmissionModeNames(), u"transmission_mode", transmission_mode);
    for (auto it : frequencies) {
        root->addElement(u"frequency")->setIntAttribute(u"value", it, false);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::ISDBTerrestrialDeliverySystemDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xfreq;
    bool ok =
        element->getIntAttribute(area_code, u"area_code", true, 0, 0, 0x0FFF) &&
        element->getEnumAttribute(guard_interval, GuardIntervalNames(), u"guard_interval", true) &&
        element->getEnumAttribute(transmission_mode, TransmissionModeNames(), u"transmission_mode", true) &&
        element->getChildren(xfreq, u"frequency", 0, 126);

    for (auto it = xfreq.begin(); ok && it != xfreq.end(); ++it) {
        uint64_t f = 0;
        ok = (*it)->getIntAttribute(f, u"value", true);
        frequencies.push_back(f);
    }
    return ok;
}
