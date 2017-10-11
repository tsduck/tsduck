//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
#include "tsToInteger.h"
#include "tsTablesDisplay.h"
#include "tsTablesFactory.h"
#include "tsXMLTables.h"
TSDUCK_SOURCE;
TS_XML_DESCRIPTOR_FACTORY(ts::SatelliteDeliverySystemDescriptor, "satellite_delivery_system_descriptor");
TS_ID_DESCRIPTOR_FACTORY(ts::SatelliteDeliverySystemDescriptor, ts::EDID(ts::DID_SAT_DELIVERY));
TS_ID_DESCRIPTOR_DISPLAY(ts::SatelliteDeliverySystemDescriptor::DisplayDescriptor, ts::EDID(ts::DID_SAT_DELIVERY));


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::SatelliteDeliverySystemDescriptor::SatelliteDeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(DID_SAT_DELIVERY, DS_DVB_S, "satellite_delivery_system_descriptor"),
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
    AbstractDeliverySystemDescriptor(DID_SAT_DELIVERY, DS_DVB_S, "satellite_delivery_system_descriptor"),
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
    const ts::Enumeration DirectionNames(
        "west", 0,
        "east", 1,
        TS_NULL);

    const ts::Enumeration PolarizationNames(
        "horizontal", 0,
        "vertical", 1,
        "left", 2,
        "right", 3,
        TS_NULL);

    const ts::Enumeration RollOffNames(
        "0.35", 0,
        "0.25", 1,
        "0.20", 2,
        "reserved", 3,
        TS_NULL);

    const ts::Enumeration SystemNames(
        "DVB-S", 0,
        "DVB-S2", 1,
        TS_NULL);

    const ts::Enumeration ModulationNames(
        "auto", 0,
        "QPSK", 1,
        "8PSK", 2,
        "16-QAM", 3,
        TS_NULL);

    const ts::Enumeration CodeRateNames(
        "undefined", 0,
        "1/2", 1,
        "2/3", 2,
        "3/4", 3,
        "5/6", 4,
        "7/8", 5,
        "8/9", 6,
        "3/5", 7,
        "4/5", 8,
        "9/10", 9,
        TS_NULL
    );
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

ts::XML::Element* ts::SatelliteDeliverySystemDescriptor::toXML(XML& xml, XML::Element* parent) const
{
    XML::Element* root = _is_valid ? xml.addElement(parent, _xml_name) : 0;
    xml.setIntAttribute(root, "frequency", 10000 * uint64_t(frequency), false);
    xml.setAttribute(root, "orbital_position", Format("%d.%d", int(orbital_position / 10), int(orbital_position % 10)));
    xml.setIntEnumAttribute(DirectionNames, root, "west_east_flag", eastNotWest);
    xml.setIntEnumAttribute(PolarizationNames, root, "polarization", polarization);
    xml.setIntEnumAttribute(RollOffNames, root, "roll_off", roll_off);
    xml.setIntEnumAttribute(SystemNames, root, "modulation_system", dvbS2);
    xml.setIntEnumAttribute(ModulationNames, root, "modulation_type", modulation_type);
    xml.setIntAttribute(root, "symbol_rate", 100 * uint64_t(symbol_rate), false);
    xml.setIntEnumAttribute(CodeRateNames, root, "FEC_inner", FEC_inner);
    return root;
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

void ts::SatelliteDeliverySystemDescriptor::fromXML(XML& xml, const XML::Element* element)
{
    uint64_t freq = 0;
    uint64_t symrate = 0;
    std::string orbit;

    _is_valid =
        checkXMLName(xml, element) &&
        xml.getIntAttribute<uint64_t>(freq, element, "frequency", true) &&
        xml.getAttribute(orbit, element, "orbital_position", true) &&
        xml.getIntEnumAttribute(eastNotWest, DirectionNames, element, "west_east_flag", true) &&
        xml.getIntEnumAttribute(polarization, PolarizationNames, element, "polarization", true) &&
        xml.getIntEnumAttribute<uint8_t>(roll_off, RollOffNames, element, "roll_off", false, 0) &&
        xml.getIntEnumAttribute(dvbS2, SystemNames, element, "modulation_system", false, false) &&
        xml.getIntEnumAttribute<uint8_t>(modulation_type, ModulationNames, element, "modulation_type", false, 1) &&
        xml.getIntAttribute<uint64_t>(symrate, element, "symbol_rate", true) &&
        xml.getIntEnumAttribute(FEC_inner, CodeRateNames, element, "FEC_inner", true);

    if (_is_valid) {
        frequency = uint32_t(freq / 10000);
        symbol_rate = uint32_t(symrate / 100);

        // Expected orbital position is "XX.X" as in "19.2".
        StringVector fields;
        uint16_t p1 = 0;
        uint16_t p2 = 0;
        SplitString(fields, orbit, '.');
        _is_valid = fields.size() == 2 && ToInteger(p1, fields[0]) && ToInteger(p2, fields[1]) && p2 < 10;
        if (_is_valid) {
            orbital_position = (p1 * 10) + p2;
        }
        else {
            xml.reportError(Format("Invalid value '%s' for attribute 'orbital_position' in <%s> at line %d, use 'nn.n'",
                                   orbit.c_str(), XML::ElementName(element), element->GetLineNum()));
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
