//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsTerrestrialDeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"terrestrial_delivery_system_descriptor"
#define MY_CLASS ts::TerrestrialDeliverySystemDescriptor
#define MY_DID ts::DID_TERREST_DELIVERY

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::TerrestrialDeliverySystemDescriptor::TerrestrialDeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(MY_DID, DS_DVB_T, MY_XML_NAME),
    centre_frequency(0),
    bandwidth(0),
    high_priority(true),
    no_time_slicing(true),
    no_mpe_fec(true),
    constellation(0),
    hierarchy(0),
    code_rate_hp(0),
    code_rate_lp(0),
    guard_interval(0),
    transmission_mode(0),
    other_frequency(false)
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

void ts::TerrestrialDeliverySystemDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt32(uint32_t(centre_frequency / 10)); // coded in 10 Hz unit
    bbp->appendUInt8(uint8_t(bandwidth << 5) |
                     uint8_t(uint8_t(high_priority) << 4) |
                     uint8_t(uint8_t(no_time_slicing) << 3) |
                     uint8_t(uint8_t(no_mpe_fec) << 2) |
                     0x03);
    bbp->appendUInt8(uint8_t(constellation << 6) |
                     uint8_t((hierarchy & 0x07) << 3) |
                     (code_rate_hp & 0x07));
    bbp->appendUInt8(uint8_t(code_rate_lp << 5) |
                     uint8_t((guard_interval & 0x03) << 3) |
                     uint8_t((transmission_mode & 0x03) << 1) |
                     uint8_t(other_frequency));
    bbp->appendUInt32(0xFFFFFFFF);
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::TerrestrialDeliverySystemDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == tag() && desc.payloadSize() >= 7;

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        centre_frequency = uint64_t(GetUInt32(data)) * 10; // coded in 10 Hz unit
        bandwidth = (data[4] >> 5) & 0x07;
        high_priority = (data[4] & 0x10) != 0;
        no_time_slicing = (data[4] & 0x08) != 0;
        no_mpe_fec = (data[4] & 0x04) != 0;
        constellation = (data[5] >> 6) & 0x03;
        hierarchy = (data[5] >> 3) & 0x07;
        code_rate_hp = data[5] & 0x07;
        code_rate_lp = (data[6] >> 5) & 0x07;
        guard_interval = (data[6] >> 3) & 0x03;
        transmission_mode = (data[6] >> 1) & 0x03;
        other_frequency = (data[6] & 0x01) != 0;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::TerrestrialDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 11) {
        uint32_t cfreq = GetUInt32(data);
        uint8_t bwidth = data[4] >> 5;
        uint8_t prio = (data[4] >> 4) & 0x01;
        uint8_t tslice = (data[4] >> 3) & 0x01;
        uint8_t mpe_fec = (data[4] >> 2) & 0x01;
        uint8_t constel = data[5] >> 6;
        uint8_t hierarchy = (data[5] >> 3) & 0x07;
        uint8_t rate_hp = data[5] & 0x07;
        uint8_t rate_lp = data[6] >> 5;
        uint8_t guard = (data[6] >> 3) & 0x03;
        uint8_t transm = (data[6] >> 1) & 0x03;
        bool other_freq = (data[6] & 0x01) != 0;
        data += 11; size -= 11;

        strm << margin << "Centre frequency: " << UString::Decimal(10 * uint64_t(cfreq)) << " Hz, Bandwidth: ";
        switch (bwidth) {
            case 0:  strm << "8 MHz"; break;
            case 1:  strm << "7 MHz"; break;
            case 2:  strm << "6 MHz"; break;
            case 3:  strm << "5 MHz"; break;
            default: strm << "code " << int(bwidth) << " (reserved)"; break;
        }
        strm << std::endl
             << margin << "Priority: " << (prio ? "high" : "low")
             << ", Time slicing: " << (tslice ? "unused" : "used")
             << ", MPE-FEC: " << (mpe_fec ? "unused" : "used")
             << std::endl
             << margin << "Constellation pattern: ";
        switch (constel) {
            case 0:  strm << "QPSK"; break;
            case 1:  strm << "16-QAM"; break;
            case 2:  strm << "64-QAM"; break;
            case 3:  strm << "reserved"; break;
            default: assert(false);
        }
        strm << std::endl << margin << "Hierarchy: ";
        assert(hierarchy < 8);
        switch (hierarchy & 0x03) {
            case 0:  strm << "non-hierarchical"; break;
            case 1:  strm << "alpha = 1"; break;
            case 2:  strm << "alpha = 2"; break;
            case 3:  strm << "alpha = 4"; break;
            default: assert(false);
        }
        strm << ", " << ((hierarchy & 0x04) ? "in-depth" : "native")
             << " interleaver" << std::endl
             << margin << "Code rate: high prio: ";
        switch (rate_hp) {
            case 0:  strm << "1/2"; break;
            case 1:  strm << "2/3"; break;
            case 2:  strm << "3/4"; break;
            case 3:  strm << "5/6"; break;
            case 4:  strm << "7/8"; break;
            default: strm << "code " << int(rate_hp) << " (reserved)"; break;
        }
        strm << ", low prio: ";
        switch (rate_lp) {
            case 0:  strm << "1/2"; break;
            case 1:  strm << "2/3"; break;
            case 2:  strm << "3/4"; break;
            case 3:  strm << "5/6"; break;
            case 4:  strm << "7/8"; break;
            default: strm << "code " << int(rate_lp) << " (reserved)"; break;
        }
        strm << std::endl << margin << "Guard interval: ";
        switch (guard) {
            case 0:  strm << "1/32"; break;
            case 1:  strm << "1/16"; break;
            case 2:  strm << "1/8"; break;
            case 3:  strm << "1/4"; break;
            default: assert(false);
        }
        strm << std::endl << margin << "OFDM transmission mode: ";
        switch (transm) {
            case 0:  strm << "2k"; break;
            case 1:  strm << "8k"; break;
            case 2:  strm << "4k"; break;
            case 3:  strm << "reserved"; break;
            default: assert(false);
        }
        strm << ", other frequencies: " << UString::YesNo(other_freq) << std::endl;
    }

    display.displayExtraData(data, size, indent);
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
    return  element->getIntAttribute<uint64_t>(centre_frequency, u"centre_frequency", true) &&
            element->getIntEnumAttribute(bandwidth, BandwidthNames, u"bandwidth", true) &&
            element->getIntEnumAttribute(high_priority, PriorityNames, u"priority", true) &&
            element->getBoolAttribute(no_time_slicing, u"no_time_slicing", true) &&
            element->getBoolAttribute(no_mpe_fec, u"no_MPE_FEC", true) &&
            element->getIntEnumAttribute(constellation, ConstellationNames, u"constellation", true) &&
            element->getIntAttribute<uint8_t>(hierarchy, u"hierarchy_information", true) &&
            element->getIntEnumAttribute(code_rate_hp, CodeRateNames, u"code_rate_HP_stream", true) &&
            element->getIntEnumAttribute(code_rate_lp, CodeRateNames, u"code_rate_LP_stream", true) &&
            element->getIntEnumAttribute(guard_interval, GuardIntervalNames, u"guard_interval", true) &&
            element->getIntEnumAttribute(transmission_mode, TransmissionModeNames, u"transmission_mode", true) &&
            element->getBoolAttribute(other_frequency, u"other_frequency", true);
}
