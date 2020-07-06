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

#include "tsSatelliteDeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsBCD.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

#define MY_XML_NAME u"satellite_delivery_system_descriptor"
#define MY_CLASS ts::SatelliteDeliverySystemDescriptor
#define MY_DID ts::DID_SAT_DELIVERY

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SatelliteDeliverySystemDescriptor::SatelliteDeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(MY_DID, DS_UNDEFINED, MY_XML_NAME),
    frequency(0),
    orbital_position(0),
    east_not_west(false),
    polarization(0),
    symbol_rate(0),
    modulation(0),
    roll_off(0),
    FEC_inner(0)
{
}

void ts::SatelliteDeliverySystemDescriptor::clearContent()
{
    frequency = 0;
    orbital_position = 0;
    east_not_west = false;
    polarization = 0;
    symbol_rate = 0;
    modulation = 0;
    roll_off = 0;
    FEC_inner = 0;
}

ts::SatelliteDeliverySystemDescriptor::SatelliteDeliverySystemDescriptor(DuckContext& duck, const Descriptor& desc) :
    SatelliteDeliverySystemDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Get / update the actual delivery system.
//----------------------------------------------------------------------------

ts::DeliverySystem ts::SatelliteDeliverySystemDescriptor::ResolveDeliverySystem(const DuckContext& duck, DeliverySystem system)
{
    if (system == DS_DVB_S || system == DS_DVB_S2 || system == DS_ISDB_S) {
        return system;
    }
    else if ((duck.standards() & Standards::ISDB) == Standards::ISDB) {
        return DS_ISDB_S;
    }
    else {
        return DS_DVB_S;
    }
}

ts::DeliverySystem ts::SatelliteDeliverySystemDescriptor::deliverySystem(const DuckContext& duck) const
{
    return ResolveDeliverySystem(duck, _system);
}

void ts::SatelliteDeliverySystemDescriptor::setDeliverySystem(const DuckContext& duck, DeliverySystem sys)
{
    _system = ResolveDeliverySystem(duck, sys);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::SatelliteDeliverySystemDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    ByteBlockPtr bbp(serializeStart());
    bbp->appendBCD(uint32_t(frequency / 10000), 8); // coded in 10 kHz units
    bbp->appendBCD(orbital_position, 4);

    // One byte is system-dependent.
    const DeliverySystem delsys = deliverySystem(duck);
    const uint8_t byte = (east_not_west ? 0x80 : 0x00) | uint8_t((polarization & 0x03) << 5);
    if (delsys == DS_ISDB_S) {
        // ISDB-S variant.
        bbp->appendUInt8(byte | (modulation & 0x1F));
    }
    else {
        // DVB-S/S2 variant.
        bbp->appendUInt8(byte |
                         (delsys == DS_DVB_S2 ? (uint8_t((roll_off & 0x03) << 3) | 0x04) : 0x18) |
                         (modulation & 0x03));
    }

    bbp->appendBCD(uint32_t(symbol_rate / 100), 7, true, FEC_inner); // coded in 100 sym/s units, FEC in last nibble
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SatelliteDeliverySystemDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    _is_valid = desc.isValid() && desc.tag() == tag() && size == 11;

    if (_is_valid) {
        frequency = 10000 * uint64_t(DecodeBCD(data, 8)); // coded in 10 kHz units
        orbital_position = uint16_t(DecodeBCD(data + 4, 4));
        east_not_west = (data[6] & 0x80) != 0;
        polarization = (data[6] >> 5) & 0x03;
        symbol_rate = 100 * uint64_t(DecodeBCD(data + 7, 7, true)); // coded in 100 sym/s units
        FEC_inner = data[10] & 0x0F;
        if ((duck.standards() & Standards::ISDB) == Standards::ISDB) {
            // ISDB-S variant.
            _system = DS_ISDB_S;
            roll_off = 0xFF;
            modulation = data[6] & 0x1F;
        }
        else {
            // DVB-S/S2 variant.
            _system = (data[6] & 0x04) != 0 ? DS_DVB_S2 : DS_DVB_S;
            roll_off = _system == DS_DVB_S2 ? ((data[6] >> 3) & 0x03) : 0xFF;
            modulation = data[6] & 0x03;
        }
    }
}


//----------------------------------------------------------------------------
// Enumerations for XML.
//----------------------------------------------------------------------------

const ts::Enumeration ts::SatelliteDeliverySystemDescriptor::DirectionNames({
    {u"west", 0},
    {u"east", 1},
});

const ts::Enumeration ts::SatelliteDeliverySystemDescriptor::PolarizationNames({
    {u"horizontal", 0},
    {u"vertical",   1},
    {u"left",       2},
    {u"right",      3},
});

const ts::Enumeration ts::SatelliteDeliverySystemDescriptor::RollOffNames({
    {u"0.35",     0},
    {u"0.25",     1},
    {u"0.20",     2},
    {u"reserved", 3},
});

const ts::Enumeration ts::SatelliteDeliverySystemDescriptor::ModulationNamesDVB({
    {u"auto",   0},
    {u"QPSK",   1},
    {u"8PSK",   2},
    {u"16-QAM", 3},
});

const ts::Enumeration ts::SatelliteDeliverySystemDescriptor::ModulationNamesISDB({
    {u"auto",         0},
    {u"QPSK",         1},
    {u"ISDB-S",       8}, // TC8PSK ?
    {u"2.6GHzMobile", 9},
    {u"AdvancedCS",  10},
});

const ts::Enumeration ts::SatelliteDeliverySystemDescriptor::CodeRateNamesDVB({
    {u"undefined", 0},
    {u"1/2",       1},
    {u"2/3",       2},
    {u"3/4",       3},
    {u"5/6",       4},
    {u"7/8",       5},
    {u"8/9",       6},
    {u"3/5",       7},
    {u"4/5",       8},
    {u"9/10",      9},
    {u"none",     15},
});

const ts::Enumeration ts::SatelliteDeliverySystemDescriptor::CodeRateNamesISDB({
    {u"undefined",    0},
    {u"1/2",          1},
    {u"2/3",          2},
    {u"3/4",          3},
    {u"5/6",          4},
    {u"7/8",          5},
    {u"ISDB-S",       8},
    {u"2.6GHzMobile", 9},
    {u"AdvancedCS",  10},
    {u"none",        15},
});


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::SatelliteDeliverySystemDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    const DeliverySystem delsys = deliverySystem(duck);
    const bool isDVB = delsys != DS_ISDB_S;

    root->setIntAttribute(u"frequency", frequency, false);
    root->setAttribute(u"orbital_position", UString::Format(u"%d.%d", {orbital_position / 10, orbital_position % 10}));
    root->setIntEnumAttribute(DirectionNames, u"west_east_flag", east_not_west);
    root->setIntEnumAttribute(PolarizationNames, u"polarization", polarization);
    if (delsys == DS_DVB_S2) {
        root->setIntEnumAttribute(RollOffNames, u"roll_off", roll_off);
    }
    root->setEnumAttribute(DeliverySystemEnum, u"modulation_system", delsys);
    root->setIntEnumAttribute(isDVB ? ModulationNamesDVB : ModulationNamesISDB, u"modulation_type", modulation);
    root->setIntAttribute(u"symbol_rate", symbol_rate, false);
    root->setIntEnumAttribute(isDVB ? CodeRateNamesDVB : CodeRateNamesISDB, u"FEC_inner", FEC_inner);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::SatelliteDeliverySystemDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    UString orbit;
    bool ok =
        element->getIntAttribute<uint64_t>(frequency, u"frequency", true) &&
        element->getAttribute(orbit, u"orbital_position", true) &&
        element->getIntEnumAttribute(east_not_west, DirectionNames, u"west_east_flag", true) &&
        element->getIntEnumAttribute(polarization, PolarizationNames, u"polarization", true) &&
        element->getIntEnumAttribute<uint8_t>(roll_off, RollOffNames, u"roll_off", false, 0) &&
        element->getIntAttribute<uint64_t>(symbol_rate, u"symbol_rate", true) &&
        element->getIntEnumAttribute<DeliverySystem>(_system, DeliverySystemEnum, u"modulation_system", true);

    if (ok) {
        // Enforce a valid delivery system (DVB-S, DVB-S2, ISDB-S).
        _system = ResolveDeliverySystem(duck, _system);
        if (_system == DS_ISDB_S) {
            // ISDB-S variant.
            // Default modulation: ISDB-S (8)
            ok = element->getIntEnumAttribute<uint8_t>(modulation, ModulationNamesISDB, u"modulation_type", false, 8) &&
                 element->getIntEnumAttribute(FEC_inner, CodeRateNamesISDB, u"FEC_inner", true);
        }
        else {
            // DVB-S/S2 variant.
            // Default modulation: QPSK (1)
            ok = element->getIntEnumAttribute<uint8_t>(modulation, ModulationNamesDVB, u"modulation_type", false, 1) &&
                 element->getIntEnumAttribute(FEC_inner, CodeRateNamesDVB, u"FEC_inner", true);
        }
    }

    if (ok) {
        // Expected orbital position is "XX.X" as in "19.2".
        UStringVector fields;
        uint16_t p1 = 0;
        uint16_t p2 = 0;
        orbit.split(fields, u'.');
        ok = fields.size() == 2 && fields[0].toInteger(p1) && fields[1].toInteger(p2) && p2 < 10;
        if (ok) {
            orbital_position = (p1 * 10) + p2;
        }
        else {
            element->report().error(u"Invalid value '%s' for attribute 'orbital_position' in <%s> at line %d, use 'nn.n'", {orbit, element->name(), element->lineNumber()});
        }
    }
    return ok;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SatelliteDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 11) {
        const bool isDVB = (duck.standards() & Standards::ISDB) == Standards::NONE;
        const uint8_t east = data[6] >> 7;
        const uint8_t polar = (data[6] >> 5) & 0x03;
        const uint8_t roll_off = (data[6] >> 3) & 0x03;
        const DeliverySystem delsys = isDVB ? (((data[6] >> 2) & 0x01) != 0 ? DS_DVB_S2 : DS_DVB_S) : DS_ISDB_S;
        const uint8_t mod_type = isDVB ? (data[6] & 0x03) : (data[6] & 0x1F);
        const uint8_t fec = data[10] & 0x0F;
        std::string freq, srate, orbital;
        BCDToString(freq, data, 8, 3);
        BCDToString(orbital, data + 4, 4, 3);
        BCDToString(srate, data + 7, 7, 3, true);
        data += 11; size -= 11;

        strm << margin << "Orbital position: " << orbital << " degree, " << (east ? "east" : "west") << std::endl
             << margin << "Frequency: " << freq << " GHz" << std::endl
             << margin << "Symbol rate: " << srate << " Msymbol/s" << std::endl
             << margin << "Polarization: " << NameFromSection(u"SatellitePolarization", polar, names::VALUE | names::DECIMAL) << std::endl
             << margin << "Delivery system: " << DeliverySystemEnum.name(delsys) << std::endl
             << margin << "Modulation: " << NameFromSection(isDVB ? u"DVBSatelliteModulationType" : u"ISDBSatelliteModulationType", mod_type, names::VALUE | names::DECIMAL);
        if (delsys == DS_DVB_S2) {
            strm << ", roll off: " << NameFromSection(u"DVBS2RollOff", roll_off, names::VALUE | names::DECIMAL);
        }
        strm << std::endl
             << margin << "Inner FEC: " << NameFromSection(isDVB ? u"DVBSatelliteFEC" : u"ISDBSatelliteFEC", fec, names::VALUE | names::DECIMAL) << std::endl;
    }

    display.displayExtraData(data, size, indent);
}
