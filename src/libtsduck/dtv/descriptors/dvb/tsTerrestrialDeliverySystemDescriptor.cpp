//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTerrestrialDeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"terrestrial_delivery_system_descriptor"
#define MY_CLASS    ts::TerrestrialDeliverySystemDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_DVB_TERREST_DELIVERY, ts::Standards::DVB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TerrestrialDeliverySystemDescriptor::TerrestrialDeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(MY_EDID, DS_DVB_T, MY_XML_NAME)
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
// Thread-safe init-safe static data patterns.
//----------------------------------------------------------------------------

const std::map<int, ts::BandWidth>& ts::TerrestrialDeliverySystemDescriptor::ToBandWidth()
{
    static const std::map<int, BandWidth> data {
        {0, 8000000},
        {1, 7000000},
        {2, 6000000},
        {3, 5000000},
    };
    return data;
}

const std::map<int, ts::Modulation>& ts::TerrestrialDeliverySystemDescriptor::ToConstellation()
{
    static const std::map<int, Modulation> data {
        {0, QPSK},
        {1, QAM_16},
        {2, QAM_64},
    };
    return data;
}

const std::map<int, ts::InnerFEC>& ts::TerrestrialDeliverySystemDescriptor::ToInnerFEC()
{
    static const std::map<int, InnerFEC> data {
        {0, FEC_1_2},
        {1, FEC_2_3},
        {2, FEC_3_4},
        {3, FEC_5_6},
        {4, FEC_7_8},
    };
    return data;
}

const std::map<int, ts::TransmissionMode>& ts::TerrestrialDeliverySystemDescriptor::ToTransmissionMode()
{
    static const std::map<int, TransmissionMode> data {
        {0, TM_2K},
        {1, TM_8K},
        {2, TM_4K},
    };
    return data;
}

const std::map<int, ts::GuardInterval>& ts::TerrestrialDeliverySystemDescriptor::ToGuardInterval()
{
    static const std::map<int, GuardInterval> data {
        {0, GUARD_1_32},
        {1, GUARD_1_16},
        {2, GUARD_1_8},
        {3, GUARD_1_4},
    };
    return data;
}

const std::map<int, ts::Hierarchy>& ts::TerrestrialDeliverySystemDescriptor::ToHierarchy()
{
    static const std::map<int, Hierarchy> data {
        {0, HIERARCHY_NONE},
        {1, HIERARCHY_1},
        {2, HIERARCHY_2},
        {3, HIERARCHY_4},
    };
    return data;
}

const ts::Names& ts::TerrestrialDeliverySystemDescriptor::BandwidthNames()
{
    static const Names data({
        {u"8MHz", 0},
        {u"7MHz", 1},
        {u"6MHz", 2},
        {u"5MHz", 3},
    });
    return data;
}

const ts::Names& ts::TerrestrialDeliverySystemDescriptor::PriorityNames()
{
    static const Names data({
        {u"HP", 1},
        {u"LP", 0},
    });
    return data;
}

const ts::Names& ts::TerrestrialDeliverySystemDescriptor::ConstellationNames()
{
    static const Names data({
        {u"QPSK",   0},
        {u"16-QAM", 1},
        {u"64-QAM", 2},
    });
    return data;
}

const ts::Names& ts::TerrestrialDeliverySystemDescriptor::CodeRateNames()
{
    static const Names data({
        {u"1/2", 0},
        {u"2/3", 1},
        {u"3/4", 2},
        {u"5/6", 3},
        {u"7/8", 4},
    });
    return data;
}

const ts::Names& ts::TerrestrialDeliverySystemDescriptor::GuardIntervalNames()
{
    static const Names data({
        {u"1/32", 0},
        {u"1/16", 1},
        {u"1/8",  2},
        {u"1/4",  3},
    });
    return data;
}

const ts::Names& ts::TerrestrialDeliverySystemDescriptor::TransmissionModeNames()
{
    static const Names data({
        {u"2k", 0},
        {u"8k", 1},
        {u"4k", 2},
    });
    return data;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::TerrestrialDeliverySystemDescriptor::serializePayload(PSIBuffer& buf) const
{
    // The frequency is coded in 10 Hz units.
    // Sometimes, the value 0xFFFFFFFF is used to say "unknown".
    buf.putUInt32(centre_frequency == 0 ? 0xFFFFFFFF : uint32_t(centre_frequency / 10));
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
    // The frequency is coded in 10 Hz units.
    // Sometimes, the value 0xFFFFFFFF is used to say "unknown".
    const uint32_t freq = buf.getUInt32();
    centre_frequency = freq == 0xFFFFFFFF ? 0 : uint64_t(freq) * 10;
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

void ts::TerrestrialDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
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
// XML serialization
//----------------------------------------------------------------------------

void ts::TerrestrialDeliverySystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"centre_frequency", centre_frequency, false);
    root->setEnumAttribute(BandwidthNames(), u"bandwidth", bandwidth);
    root->setEnumAttribute(PriorityNames(), u"priority", int(high_priority));
    root->setBoolAttribute(u"no_time_slicing", no_time_slicing);
    root->setBoolAttribute(u"no_MPE_FEC", no_mpe_fec);
    root->setEnumAttribute(ConstellationNames(), u"constellation", constellation);
    root->setIntAttribute(u"hierarchy_information", hierarchy);
    root->setEnumAttribute(CodeRateNames(), u"code_rate_HP_stream", code_rate_hp);
    root->setEnumAttribute(CodeRateNames(), u"code_rate_LP_stream", code_rate_lp);
    root->setEnumAttribute(GuardIntervalNames(), u"guard_interval", guard_interval);
    root->setEnumAttribute(TransmissionModeNames(), u"transmission_mode", transmission_mode);
    root->setBoolAttribute(u"other_frequency", other_frequency);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::TerrestrialDeliverySystemDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(centre_frequency, u"centre_frequency", true) &&
           element->getEnumAttribute(bandwidth, BandwidthNames(), u"bandwidth", true) &&
           element->getEnumAttribute(high_priority, PriorityNames(), u"priority", true) &&
           element->getBoolAttribute(no_time_slicing, u"no_time_slicing", true) &&
           element->getBoolAttribute(no_mpe_fec, u"no_MPE_FEC", true) &&
           element->getEnumAttribute(constellation, ConstellationNames(), u"constellation", true) &&
           element->getIntAttribute(hierarchy, u"hierarchy_information", true) &&
           element->getEnumAttribute(code_rate_hp, CodeRateNames(), u"code_rate_HP_stream", true) &&
           element->getEnumAttribute(code_rate_lp, CodeRateNames(), u"code_rate_LP_stream", true) &&
           element->getEnumAttribute(guard_interval, GuardIntervalNames(), u"guard_interval", true) &&
           element->getEnumAttribute(transmission_mode, TransmissionModeNames(), u"transmission_mode", true) &&
           element->getBoolAttribute(other_frequency, u"other_frequency", true);
}
