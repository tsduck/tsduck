//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTerrestrialDeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"terrestrial_delivery_system_descriptor"
#define MY_CLASS ts::TerrestrialDeliverySystemDescriptor
#define MY_DID ts::DID_TERREST_DELIVERY

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TerrestrialDeliverySystemDescriptor::TerrestrialDeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(MY_DID, DS_DVB_T, MY_XML_NAME)
{
}

ts::TerrestrialDeliverySystemDescriptor::TerrestrialDeliverySystemDescriptor(DuckContext& duck, const Descriptor& desc) :
    TerrestrialDeliverySystemDescriptor()
{
    deserialize(duck, desc);
}

void ts::TerrestrialDeliverySystemDescriptor::clearContent()
{
    centre_frequency = 0;
    bandwidth = 0;
    high_priority = true;
    no_time_slicing = true;
    no_mpe_fec = true;
    constellation = 0;
    hierarchy = 0;
    code_rate_hp = 0;
    code_rate_lp = 0;
    guard_interval = 0;
    transmission_mode = 0;
    other_frequency = false;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TerrestrialDeliverySystemDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt32(uint32_t(centre_frequency / 10)); // coded in 10 Hz unit
    buf.putBits(bandwidth, 3);
    buf.putBit(high_priority);
    buf.putBit(no_time_slicing);
    buf.putBit(no_mpe_fec);
    buf.putBits(0xFF, 2);
    buf.putBits(constellation, 2);
    buf.putBits(hierarchy, 3);
    buf.putBits(code_rate_hp, 3);
    buf.putBits(code_rate_lp, 3);
    buf.putBits(guard_interval, 2);
    buf.putBits(transmission_mode, 2);
    buf.putBit(other_frequency);
    buf.putUInt32(0xFFFFFFFF);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TerrestrialDeliverySystemDescriptor::deserializePayload(PSIBuffer& buf)
{
    centre_frequency = uint64_t(buf.getUInt32()) * 10; // coded in 10 Hz unit
    buf.getBits(bandwidth, 3);
    high_priority = buf.getBool();
    no_time_slicing = buf.getBool();
    no_mpe_fec = buf.getBool();
    buf.skipReservedBits(2);
    buf.getBits(constellation, 2);
    buf.getBits(hierarchy, 3);
    buf.getBits(code_rate_hp, 3);
    buf.getBits(code_rate_lp, 3);
    buf.getBits(guard_interval, 2);
    buf.getBits(transmission_mode, 2);
    other_frequency = buf.getBool();
    buf.skipReservedBits(32);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TerrestrialDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(11)) {
        disp << margin << "Centre frequency: " << UString::Decimal(10 * uint64_t(buf.getUInt32())) << " Hz, Bandwidth: ";
        const uint8_t bwidth = buf.getBits<uint8_t>(3);
        switch (bwidth) {
            case 0:  disp << "8 MHz"; break;
            case 1:  disp << "7 MHz"; break;
            case 2:  disp << "6 MHz"; break;
            case 3:  disp << "5 MHz"; break;
            default: disp << "code " << int(bwidth) << " (reserved)"; break;
        }
        disp << std::endl;
        disp << margin << "Priority: " << (buf.getBool() ? "high" : "low");
        disp << ", Time slicing: " << (buf.getBool() ? "unused" : "used");
        disp << ", MPE-FEC: " << (buf.getBool() ? "unused" : "used") << std::endl;
        buf.skipReservedBits(2);
        disp << margin << "Constellation pattern: ";
        switch (buf.getBits<uint8_t>(2)) {
            case 0:  disp << "QPSK"; break;
            case 1:  disp << "16-QAM"; break;
            case 2:  disp << "64-QAM"; break;
            case 3:  disp << "reserved"; break;
            default: assert(false);
        }
        disp << std::endl;
        disp << margin << "Hierarchy: ";
        const uint8_t hierarchy = buf.getBits<uint8_t>(3);
        switch (hierarchy & 0x03) {
            case 0:  disp << "non-hierarchical"; break;
            case 1:  disp << "alpha = 1"; break;
            case 2:  disp << "alpha = 2"; break;
            case 3:  disp << "alpha = 4"; break;
            default: assert(false);
        }
        disp << ", " << ((hierarchy & 0x04) ? "in-depth" : "native") << " interleaver" << std::endl;
        const uint8_t rate_hp = buf.getBits<uint8_t>(3);
        disp << margin << "Code rate: high prio: ";
        switch (rate_hp) {
            case 0:  disp << "1/2"; break;
            case 1:  disp << "2/3"; break;
            case 2:  disp << "3/4"; break;
            case 3:  disp << "5/6"; break;
            case 4:  disp << "7/8"; break;
            default: disp << "code " << int(rate_hp) << " (reserved)"; break;
        }
        const uint8_t rate_lp = buf.getBits<uint8_t>(3);
        disp << ", low prio: ";
        switch (rate_lp) {
            case 0:  disp << "1/2"; break;
            case 1:  disp << "2/3"; break;
            case 2:  disp << "3/4"; break;
            case 3:  disp << "5/6"; break;
            case 4:  disp << "7/8"; break;
            default: disp << "code " << int(rate_lp) << " (reserved)"; break;
        }
        disp << std::endl;
        disp << margin << "Guard interval: ";
        switch (buf.getBits<uint8_t>(2)) {
            case 0:  disp << "1/32"; break;
            case 1:  disp << "1/16"; break;
            case 2:  disp << "1/8"; break;
            case 3:  disp << "1/4"; break;
            default: assert(false);
        }
        disp << std::endl;
        disp << margin << "OFDM transmission mode: ";
        switch (buf.getBits<uint8_t>(2)) {
            case 0:  disp << "2k"; break;
            case 1:  disp << "8k"; break;
            case 2:  disp << "4k"; break;
            case 3:  disp << "reserved"; break;
            default: assert(false);
        }
        disp << ", other frequencies: " << UString::YesNo(buf.getBool()) << std::endl;
        buf.skipReservedBits(32);
    }
}


//----------------------------------------------------------------------------
// Enumerations in XML data.
//----------------------------------------------------------------------------

namespace {
    const ts::Enumeration BandwidthNames({
        {u"8MHz", 0},
        {u"7MHz", 1},
        {u"6MHz", 2},
        {u"5MHz", 3},
    });
    const ts::Enumeration PriorityNames({
        {u"HP", 1},
        {u"LP", 0},
    });
    const ts::Enumeration ConstellationNames({
        {u"QPSK",   0},
        {u"16-QAM", 1},
        {u"64-QAM", 2},
    });
    const ts::Enumeration CodeRateNames({
        {u"1/2", 0},
        {u"2/3", 1},
        {u"3/4", 2},
        {u"5/6", 3},
        {u"7/8", 4},
    });
    const ts::Enumeration GuardIntervalNames({
        {u"1/32", 0},
        {u"1/16", 1},
        {u"1/8",  2},
        {u"1/4",  3},
    });
    const ts::Enumeration TransmissionModeNames({
        {u"2k", 0},
        {u"8k", 1},
        {u"4k", 2},
    });
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::TerrestrialDeliverySystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"centre_frequency", centre_frequency, false);
    root->setIntEnumAttribute(BandwidthNames, u"bandwidth", bandwidth);
    root->setIntEnumAttribute(PriorityNames, u"priority", int(high_priority));
    root->setBoolAttribute(u"no_time_slicing", no_time_slicing);
    root->setBoolAttribute(u"no_MPE_FEC", no_mpe_fec);
    root->setIntEnumAttribute(ConstellationNames, u"constellation", constellation);
    root->setIntAttribute(u"hierarchy_information", hierarchy);
    root->setIntEnumAttribute(CodeRateNames, u"code_rate_HP_stream", code_rate_hp);
    root->setIntEnumAttribute(CodeRateNames, u"code_rate_LP_stream", code_rate_lp);
    root->setIntEnumAttribute(GuardIntervalNames, u"guard_interval", guard_interval);
    root->setIntEnumAttribute(TransmissionModeNames, u"transmission_mode", transmission_mode);
    root->setBoolAttribute(u"other_frequency", other_frequency);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TerrestrialDeliverySystemDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return  element->getIntAttribute(centre_frequency, u"centre_frequency", true) &&
            element->getIntEnumAttribute(bandwidth, BandwidthNames, u"bandwidth", true) &&
            element->getIntEnumAttribute(high_priority, PriorityNames, u"priority", true) &&
            element->getBoolAttribute(no_time_slicing, u"no_time_slicing", true) &&
            element->getBoolAttribute(no_mpe_fec, u"no_MPE_FEC", true) &&
            element->getIntEnumAttribute(constellation, ConstellationNames, u"constellation", true) &&
            element->getIntAttribute(hierarchy, u"hierarchy_information", true) &&
            element->getIntEnumAttribute(code_rate_hp, CodeRateNames, u"code_rate_HP_stream", true) &&
            element->getIntEnumAttribute(code_rate_lp, CodeRateNames, u"code_rate_LP_stream", true) &&
            element->getIntEnumAttribute(guard_interval, GuardIntervalNames, u"guard_interval", true) &&
            element->getIntEnumAttribute(transmission_mode, TransmissionModeNames, u"transmission_mode", true) &&
            element->getBoolAttribute(other_frequency, u"other_frequency", true);
}
