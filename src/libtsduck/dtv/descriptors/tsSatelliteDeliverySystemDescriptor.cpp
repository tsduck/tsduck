//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSatelliteDeliverySystemDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"satellite_delivery_system_descriptor"
#define MY_CLASS ts::SatelliteDeliverySystemDescriptor
#define MY_DID ts::DID_SAT_DELIVERY

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::SatelliteDeliverySystemDescriptor::SatelliteDeliverySystemDescriptor() :
    AbstractDeliverySystemDescriptor(MY_DID, DS_UNDEFINED, MY_XML_NAME)
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
    else if (bool(duck.standards() & Standards::ISDB)) {
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

void ts::SatelliteDeliverySystemDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBCD(frequency / 10000, 8); // coded in 10 kHz units
    buf.putBCD(orbital_position, 4);
    buf.putBit(east_not_west);
    buf.putBits(polarization, 2);

    // 5 bits are system-dependent (DVB vs. ISDB)
    const DeliverySystem delsys = deliverySystem(buf.duck());
    if (delsys == DS_ISDB_S) {
        // ISDB-S variant.
        buf.putBits(modulation, 5);
    }
    else {
        // DVB-S/S2 variant.
        buf.putBits(delsys == DS_DVB_S2 ? roll_off : 0, 2);
        buf.putBit(delsys == DS_DVB_S2);
        buf.putBits(modulation, 2);
    }

    buf.putBCD(symbol_rate / 100, 7); // coded in 100 sym/s units
    buf.putBits(FEC_inner, 4);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::SatelliteDeliverySystemDescriptor::deserializePayload(PSIBuffer& buf)
{
    frequency = 10000 * buf.getBCD<uint64_t>(8); // coded in 10 kHz units
    orbital_position = buf.getBCD<uint16_t>(4);
    east_not_west = buf.getBool();
    buf.getBits(polarization, 2);

    if (bool(buf.duck().standards() & Standards::ISDB)) {
        // ISDB-S variant.
        _system = DS_ISDB_S;
        buf.getBits(modulation, 5);
    }
    else {
        // DVB-S/S2 variant.
        buf.getBits(roll_off, 2);
        _system = buf.getBool() ? DS_DVB_S2 : DS_DVB_S;
        buf.getBits(modulation, 2);
    }
    if (_system != DS_DVB_S2) {
        roll_off = 0xFF;
    }
    symbol_rate = 100 * buf.getBCD<uint64_t>(7); // coded in 100 sym/s units
    buf.getBits(FEC_inner, 4);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::SatelliteDeliverySystemDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(11)) {
        disp << margin << UString::Format(u"Frequency: %d", {buf.getBCD<uint32_t>(3)});
        disp << UString::Format(u".%05d GHz", {buf.getBCD<uint32_t>(5)}) << std::endl;
        disp << margin << UString::Format(u"Orbital position: %d", {buf.getBCD<uint32_t>(3)});
        disp << UString::Format(u".%d degree, ", {buf.getBCD<uint32_t>(1)});
        disp << (buf.getBool() ? "east" : "west") << std::endl;
        disp << margin << "Polarization: " << DataName(MY_XML_NAME, u"Polarization", buf.getBits<uint8_t>(2), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
        const bool isdb = bool(disp.duck().standards() & Standards::ISDB);
        if (isdb) {
            disp << margin << "Delivery system: " << DeliverySystemEnum.name(DS_ISDB_S) << std::endl;
            disp << margin << "Modulation: " << DataName(MY_XML_NAME, u"ISDBModulation", buf.getBits<uint8_t>(5), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
        }
        else {
            const uint8_t roll_off = buf.getBits<uint8_t>(2);
            const bool s2 = buf.getBool();
            disp << margin << "Delivery system: " << DeliverySystemEnum.name(s2 ? DS_DVB_S2 : DS_DVB_S) << std::endl;
            disp << margin << "Modulation: " << DataName(MY_XML_NAME, u"DVBModulation", buf.getBits<uint8_t>(2), NamesFlags::VALUE | NamesFlags::DECIMAL);
            if (s2) {
                disp << ", roll off: " << DataName(MY_XML_NAME, u"DVBS2RollOff", roll_off, NamesFlags::VALUE | NamesFlags::DECIMAL);
            }
            disp << std::endl;
        }
        disp << margin << UString::Format(u"Symbol rate: %d", {buf.getBCD<uint32_t>(3)});
        disp << UString::Format(u".%04d Msymbol/s", {buf.getBCD<uint32_t>(4)}) << std::endl;
        disp << margin << "Inner FEC: " << DataName(MY_XML_NAME, isdb ? u"ISDBSatelliteFEC" : u"DVBSatelliteFEC", buf.getBits<uint8_t>(4), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
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
        element->getIntAttribute(frequency, u"frequency", true) &&
        element->getAttribute(orbit, u"orbital_position", true) &&
        element->getIntEnumAttribute(east_not_west, DirectionNames, u"west_east_flag", true) &&
        element->getIntEnumAttribute(polarization, PolarizationNames, u"polarization", true) &&
        element->getIntEnumAttribute(roll_off, RollOffNames, u"roll_off", false, 0) &&
        element->getIntAttribute(symbol_rate, u"symbol_rate", true) &&
        element->getIntEnumAttribute<DeliverySystem>(_system, DeliverySystemEnum, u"modulation_system", true);

    if (ok) {
        // Enforce a valid delivery system (DVB-S, DVB-S2, ISDB-S).
        _system = ResolveDeliverySystem(duck, _system);
        if (_system == DS_ISDB_S) {
            // ISDB-S variant.
            // Default modulation: ISDB-S (8)
            ok = element->getIntEnumAttribute(modulation, ModulationNamesISDB, u"modulation_type", false, 8) &&
                 element->getIntEnumAttribute(FEC_inner, CodeRateNamesISDB, u"FEC_inner", true);
        }
        else {
            // DVB-S/S2 variant.
            // Default modulation: QPSK (1)
            ok = element->getIntEnumAttribute(modulation, ModulationNamesDVB, u"modulation_type", false, 1) &&
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
