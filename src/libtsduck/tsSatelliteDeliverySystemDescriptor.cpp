//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//
//  Representation of a satellite_delivery_system_descriptor
//
//----------------------------------------------------------------------------

#include "tsSatelliteDeliverySystemDescriptor.h"
#include "tsBCD.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"satellite_delivery_system_descriptor"
#define MY_DID ts::DID_SAT_DELIVERY

TS_XML_DESCRIPTOR_FACTORY(ts::SatelliteDeliverySystemDescriptor, MY_XML_NAME);
TS_ID_DESCRIPTOR_FACTORY(ts::SatelliteDeliverySystemDescriptor, ts::EDID::Standard(MY_DID));
TS_ID_DESCRIPTOR_DISPLAY(ts::SatelliteDeliverySystemDescriptor::DisplayDescriptor, ts::EDID::Standard(MY_DID));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::SatelliteDeliverySystemDescriptor::SatelliteDeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(MY_DID, DS_DVB_S, MY_XML_NAME),
    frequency(0),
    orbital_position(0),
    eastNotWest(false),
    polarization(0),
    roll_off(0),
    dvbS2(false),
    modulation_type(0),
    symbol_rate(0),
    FEC_inner(0)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::SatelliteDeliverySystemDescriptor::SatelliteDeliverySystemDescriptor(const Descriptor& desc, const DVBCharset* charset) :
    AbstractDeliverySystemDescriptor(MY_DID, DS_DVB_S, MY_XML_NAME),
    frequency(0),
    orbital_position(0),
    eastNotWest(false),
    polarization(0),
    roll_off(0),
    dvbS2(false),
    modulation_type(0),
    symbol_rate(0),
    FEC_inner(0)
{
    deserialize(desc, charset);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SatelliteDeliverySystemDescriptor::serialize (Descriptor& desc, const DVBCharset* charset) const
{
    uint8_t data[13];
    data[0] = _tag;
    data[1] = 11;
    EncodeBCD(data + 2, 8, frequency);
    EncodeBCD(data + 6, 4, orbital_position);
    data[8] = (eastNotWest ? 0x80 : 0x00) |
        ((polarization & 0x03) << 5) |
        (dvbS2 ? ((roll_off & 0x03) << 3) : 0x00) |
        (dvbS2 ? 0x04 : 0x00) |
        (modulation_type & 0x03);
    EncodeBCD(data + 9, 7, symbol_rate);
    data[12] = (data[12] & 0xF0) | (FEC_inner & 0x0F);

    Descriptor d (data, sizeof(data));
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SatelliteDeliverySystemDescriptor::deserialize (const Descriptor& desc, const DVBCharset* charset)
{
    if (!(_is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() == 11)) {
        return;
    }

    const uint8_t* data = desc.payload();

    frequency = DecodeBCD(data, 8);
    orbital_position = uint16_t(DecodeBCD(data + 4, 4));
    eastNotWest = (data[6] & 0x80) != 0;
    polarization = (data[6] >> 5) & 0x03;
    dvbS2 = (data[6] & 0x04) != 0;
    roll_off = dvbS2 ? ((data[6] >> 3) & 0x03) : 0x00;
    modulation_type = data[6] & 0x03;
    symbol_rate = DecodeBCD(data + 7, 7);
    FEC_inner = data[10] & 0x0F;
}


//----------------------------------------------------------------------------
// Enumerations for XML.
//----------------------------------------------------------------------------

namespace {
    const ts::Enumeration DirectionNames({
        {u"west", 0},
        {u"east", 1},
    });

    const ts::Enumeration PolarizationNames({
        {u"horizontal", 0},
        {u"vertical", 1},
        {u"left", 2},
        {u"right", 3},
    });

    const ts::Enumeration RollOffNames({
        {u"0.35", 0},
        {u"0.25", 1},
        {u"0.20", 2},
        {u"reserved", 3},
    });

    const ts::Enumeration SystemNames({
        {u"DVB-S", 0},
        {u"DVB-S2", 1},
    });

    const ts::Enumeration ModulationNames({
        {u"auto", 0},
        {u"QPSK", 1},
        {u"8PSK", 2},
        {u"16-QAM", 3},
    });

    const ts::Enumeration CodeRateNames({
        {u"undefined", 0},
        {u"1/2", 1},
        {u"2/3", 2},
        {u"3/4", 3},
        {u"5/6", 4},
        {u"7/8", 5},
        {u"8/9", 6},
        {u"3/5", 7},
        {u"4/5", 8},
        {u"9/10", 9},
    });
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SatelliteDeliverySystemDescriptor::buildXML(xml::Element* root) const
{
    root->setIntAttribute(u"frequency", 10000 * uint64_t(frequency), false);
    root->setAttribute(u"orbital_position", UString::Format(u"%d.%d", {orbital_position / 10, orbital_position % 10}));
    root->setIntEnumAttribute(DirectionNames, u"west_east_flag", eastNotWest);
    root->setIntEnumAttribute(PolarizationNames, u"polarization", polarization);
    root->setIntEnumAttribute(RollOffNames, u"roll_off", roll_off);
    root->setIntEnumAttribute(SystemNames, u"modulation_system", dvbS2);
    root->setIntEnumAttribute(ModulationNames, u"modulation_type", modulation_type);
    root->setIntAttribute(u"symbol_rate", 100 * uint64_t(symbol_rate), false);
    root->setIntEnumAttribute(CodeRateNames, u"FEC_inner", FEC_inner);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::SatelliteDeliverySystemDescriptor::fromXML(const xml::Element* element)
{
    uint64_t freq = 0;
    uint64_t symrate = 0;
    UString orbit;

    _is_valid =
        checkXMLName(element) &&
        element->getIntAttribute<uint64_t>(freq, u"frequency", true) &&
        element->getAttribute(orbit, u"orbital_position", true) &&
        element->getIntEnumAttribute(eastNotWest, DirectionNames, u"west_east_flag", true) &&
        element->getIntEnumAttribute(polarization, PolarizationNames, u"polarization", true) &&
        element->getIntEnumAttribute<uint8_t>(roll_off, RollOffNames, u"roll_off", false, 0) &&
        element->getIntEnumAttribute(dvbS2, SystemNames, u"modulation_system", false, false) &&
        element->getIntEnumAttribute<uint8_t>(modulation_type, ModulationNames, u"modulation_type", false, 1) &&
        element->getIntAttribute<uint64_t>(symrate, u"symbol_rate", true) &&
        element->getIntEnumAttribute(FEC_inner, CodeRateNames, u"FEC_inner", true);

    if (_is_valid) {
        frequency = uint32_t(freq / 10000);
        symbol_rate = uint32_t(symrate / 100);

        // Expected orbital position is "XX.X" as in "19.2".
        UStringVector fields;
        uint16_t p1 = 0;
        uint16_t p2 = 0;
        orbit.split(fields, u'.');
        _is_valid = fields.size() == 2 && fields[0].toInteger(p1) && fields[1].toInteger(p2) && p2 < 10;
        if (_is_valid) {
            orbital_position = (p1 * 10) + p2;
        }
        else {
            element->report().error(u"Invalid value '%s' for attribute 'orbital_position' in <%s> at line %d, use 'nn.n'", {orbit, element->name(), element->lineNumber()});
        }
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SatelliteDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    std::ostream& strm(display.out());
    const std::string margin(indent, ' ');

    if (size >= 11) {
        const uint8_t east = data[6] >> 7;
        const uint8_t polar = (data[6] >> 5) & 0x03;
        const uint8_t roll_off = (data[6] >> 3) & 0x03;
        const uint8_t mod_system = (data[6] >> 2) & 0x01;
        const uint8_t mod_type = data[6] & 0x03;
        const uint8_t fec_inner = data[10] & 0x0F;
        std::string freq, srate, orbital;
        BCDToString(freq, data, 8, 3);
        BCDToString(orbital, data + 4, 4, 3);
        BCDToString(srate, data + 7, 7, 3);
        data += 11; size -= 11;

        strm << margin << "Orbital position: " << orbital
             << " degree, " << (east ? "east" : "west") << std::endl
             << margin << "Frequency: " << freq << " GHz" << std::endl
             << margin << "Symbol rate: " << srate << " Msymbol/s" << std::endl
             << margin << "Polarization: ";
        switch (polar) {
            case 0:  strm << "linear - horizontal"; break;
            case 1:  strm << "linear - vertical"; break;
            case 2:  strm << "circular - left"; break;
            case 3:  strm << "circular - right"; break;
            default: assert(false);
        }
        strm << std::endl << margin << "Modulation: " << (mod_system == 0 ? "DVB-S" : "DVB-S2") << ", ";
        switch (mod_type) {
            case 0:  strm << "Auto"; break;
            case 1:  strm << "QPSK"; break;
            case 2:  strm << "8PSK"; break;
            case 3:  strm << "16-QAM"; break;
            default: assert(false);
        }
        if (mod_system == 1) {
            switch (roll_off) {
                case 0:  strm << ", alpha=0.35"; break;
                case 1:  strm << ", alpha=0.25"; break;
                case 2:  strm << ", alpha=0.20"; break;
                case 3:  strm << ", undefined roll-off (3)"; break;
                default: assert(false);
            }
        }
        strm << std::endl << margin << "Inner FEC: ";
        switch (fec_inner) {
            case 0:  strm << "not defined"; break;
            case 1:  strm << "1/2"; break;
            case 2:  strm << "2/3"; break;
            case 3:  strm << "3/4"; break;
            case 4:  strm << "5/6"; break;
            case 5:  strm << "7/8"; break;
            case 6:  strm << "8/9"; break;
            case 7:  strm << "3/5"; break;
            case 8:  strm << "4/5"; break;
            case 9:  strm << "9/10"; break;
            case 15: strm << "none"; break;
            default: strm << "code " << int(fec_inner) << " (reserved)"; break;
        }
        strm << std::endl;
    }

    display.displayExtraData(data, size, indent);
}
