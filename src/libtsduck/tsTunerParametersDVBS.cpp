//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//  DVB-S / DVB-S2 (satellite) tuners parameters
//
//----------------------------------------------------------------------------

#include "tsTunerParametersDVBS.h"
#include "tsTunerArgs.h"
#include "tsDektec.h"
#include "tsEnumeration.h"
#include "tsxmlElement.h"
#include "tsBCD.h"
TSDUCK_SOURCE;

#if defined (TS_NEED_STATIC_CONST_DEFINITIONS)
const ts::Polarization ts::TunerParametersDVBS::DEFAULT_POLARITY;
const ts::SpectralInversion ts::TunerParametersDVBS::DEFAULT_INVERSION;
const uint32_t ts::TunerParametersDVBS::DEFAULT_SYMBOL_RATE;
const ts::InnerFEC ts::TunerParametersDVBS::DEFAULT_INNER_FEC;
const size_t ts::TunerParametersDVBS::DEFAULT_SATELLITE_NUMBER;
const ts::DeliverySystem ts::TunerParametersDVBS::DEFAULT_DELIVERY_SYSTEM;
const ts::Modulation ts::TunerParametersDVBS::DEFAULT_MODULATION;
const ts::Pilot ts::TunerParametersDVBS::DEFAULT_PILOTS;
const ts::RollOff ts::TunerParametersDVBS::DEFAULT_ROLL_OFF;
#endif


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TunerParametersDVBS::TunerParametersDVBS() :
    TunerParameters (DVB_S),
    frequency (0),
    polarity (DEFAULT_POLARITY),
    lnb (LNB::Universal),
    inversion (DEFAULT_INVERSION),
    symbol_rate (DEFAULT_SYMBOL_RATE),
    inner_fec (DEFAULT_INNER_FEC),
    satellite_number (DEFAULT_SATELLITE_NUMBER),
    delivery_system (DEFAULT_DELIVERY_SYSTEM),
    modulation (DEFAULT_MODULATION),
    pilots (DEFAULT_PILOTS),
    roll_off (DEFAULT_ROLL_OFF)
{
}


//----------------------------------------------------------------------------
// Virtual assignment
//----------------------------------------------------------------------------

void ts::TunerParametersDVBS::copy(const TunerParameters& obj)
{
    const TunerParametersDVBS* other = dynamic_cast <const TunerParametersDVBS*> (&obj);
    if (other == nullptr) {
        throw IncompatibleTunerParametersError(u"DVBS != " + TunerTypeEnum.name(obj.tunerType()));
    }
    else {
        this->frequency = other->frequency;
        this->polarity = other->polarity;
        this->lnb = other->lnb;
        this->inversion = other->inversion;
        this->symbol_rate = other->symbol_rate;
        this->inner_fec = other->inner_fec;
        this->satellite_number = other->satellite_number;
        this->delivery_system = other->delivery_system;
        this->modulation = other->modulation;
        this->pilots = other->pilots;
        this->roll_off = other->roll_off;
    }
}


//----------------------------------------------------------------------------
// Format the tuner parameters as a list of options for the dvb tsp plugin.
//----------------------------------------------------------------------------

ts::UString ts::TunerParametersDVBS::toPluginOptions(bool no_local) const
{
    UString local_options;
    if (!no_local) {
        local_options.format(u" --lnb %s --satellite-number %d", {UString(lnb), satellite_number});
    }

    return UString::Format(u"--frequency %d --symbol-rate %d", {frequency, symbol_rate}) +
        u" --fec-inner " + InnerFECEnum.name(inner_fec) +
        u" --spectral-inversion " + SpectralInversionEnum.name(inversion) +
        u" --polarity " + PolarizationEnum.name(polarity) +
        u" --delivery-system " + DeliverySystemEnum.name(delivery_system) +
        u" --modulation " + ModulationEnum.name(modulation) +
        u" --pilots " + PilotEnum.name(pilots) +
        u" --roll-off " + RollOffEnum.name(roll_off) +
        local_options;
}


//----------------------------------------------------------------------------
// Format a short description (frequency and essential parameters).
//----------------------------------------------------------------------------

ts::UString ts::TunerParametersDVBS::shortDescription(int strength, int quality) const
{
    UString desc = UString::Decimal(frequency) + u" Hz";
    switch (polarity) {
        case POL_HORIZONTAL:
            desc += u" H";
            break;
        case POL_VERTICAL:
            desc += u" V";
            break;
        case POL_LEFT:
            desc += u" L";
            break;
        case POL_RIGHT:
            desc += u" R";
            break;
        default:
            break;
    }
    if (delivery_system != DS_DVB_S) {
        desc += u" (" + DeliverySystemEnum.name(delivery_system);
        if (modulation != QAM_AUTO) {
            desc += u", " + ModulationEnum.name(modulation);
        }
        desc += u")";
    }
    if (strength >= 0) {
        desc += UString::Format(u", strength: %d%%", {strength});
    }
    if (quality >= 0) {
        desc += UString::Format(u", quality: %d%%", {quality});
    }
    return desc;
}


//----------------------------------------------------------------------------
// Display a description of the modulation paramters on a stream, line by line.
//----------------------------------------------------------------------------

void ts::TunerParametersDVBS::displayParameters(std::ostream& strm, const UString& margin, bool verbose) const
{
    if (verbose || delivery_system != DS_DVB_S) {
        strm << margin << "Delivery system: " << DeliverySystemEnum.name(delivery_system) << std::endl;
    }
    if (frequency != 0) {
        strm << margin << "Carrier frequency: " << UString::Decimal(frequency) << " Hz" << std::endl;
    }
    if (polarity != POL_AUTO) {
        strm << margin << "Polarity: " << PolarizationEnum.name(polarity) << std::endl;
    }
    if (inversion != SPINV_AUTO) {
        strm << margin << "Spectral inversion: " << SpectralInversionEnum.name(inversion) << std::endl;
    }
    if (symbol_rate != 0) {
        strm << margin << "Symbol rate: " << UString::Decimal(symbol_rate) << " symb/s" << std::endl;
    }
    if ((verbose || delivery_system != DS_DVB_S) && modulation != QAM_AUTO) {
        strm << margin << "Modulation: " << ModulationEnum.name(modulation) << std::endl;
    }
    if (inner_fec != ts::FEC_AUTO) {
        strm << margin << "FEC inner: " << InnerFECEnum.name(inner_fec) << std::endl;
    }
    if ((verbose || delivery_system != DS_DVB_S) && pilots != PILOT_AUTO) {
        strm << margin << "Pilots: " << PilotEnum.name(pilots) << std::endl;
    }
    if ((verbose || delivery_system != DS_DVB_S) && roll_off != ROLLOFF_AUTO) {
        strm << margin << "Roll-off: " << RollOffEnum.name(roll_off) << std::endl;
    }
    if (verbose) {
        strm << margin << "LNB: " << UString(lnb) << std::endl;
    }
    if (verbose) {
        strm << margin << "Satellite number: " << satellite_number << std::endl;
    }
}


//----------------------------------------------------------------------------
// Extract options from a TunerArgs, applying defaults when necessary.
//----------------------------------------------------------------------------

bool ts::TunerParametersDVBS::fromArgs (const TunerArgs& tuner, Report& report)
{
    if (!tuner.frequency.set()) {
        report.error(u"no frequency specified, use option --frequency");
        return false;
    }

    frequency = tuner.frequency.value();
    symbol_rate = tuner.symbol_rate.set() ? tuner.symbol_rate.value() : DEFAULT_SYMBOL_RATE;
    inner_fec = tuner.inner_fec.set() ? tuner.inner_fec.value() : DEFAULT_INNER_FEC;
    inversion = tuner.inversion.set() ? tuner.inversion.value() : DEFAULT_INVERSION;
    polarity = tuner.polarity.set() ? tuner.polarity.value() : DEFAULT_POLARITY;
    satellite_number = tuner.satellite_number.set() ? tuner.satellite_number.value() : DEFAULT_SATELLITE_NUMBER;
    lnb = tuner.lnb.set() ? tuner.lnb.value() : LNB::Universal;
    delivery_system = tuner.delivery_system.set() ? tuner.delivery_system.value() : DEFAULT_DELIVERY_SYSTEM;
    modulation = tuner.modulation.set() ? tuner.modulation.value() : DEFAULT_MODULATION;
    pilots = tuner.pilots.set() ? tuner.pilots.value() : DEFAULT_PILOTS;
    roll_off = tuner.roll_off.set() ? tuner.roll_off.value() : DEFAULT_ROLL_OFF;

    return true;
}


//----------------------------------------------------------------------------
// Extract tuning information from a delivery descriptor.
//----------------------------------------------------------------------------

bool ts::TunerParametersDVBS::fromDeliveryDescriptor(const Descriptor& desc)
{
    if (!desc.isValid() || desc.tag() != DID_SAT_DELIVERY || desc.payloadSize() < 11) {
        return false;
    }

    const uint8_t* data = desc.payload();
    frequency = uint64_t(DecodeBCD(data, 8)) * 10000;
    symbol_rate = DecodeBCD(data + 7, 7) * 100;

    // Polarity.
    switch ((data[6] >> 5) & 0x03) {
        case 0: polarity = POL_HORIZONTAL; break;
        case 1: polarity = POL_VERTICAL; break;
        case 2: polarity = POL_LEFT; break;
        case 3: polarity = POL_RIGHT; break;
        default: assert (false);
    }

    // Inner FEC.
    switch (data[10] & 0x0F) {
        case 1:  inner_fec = FEC_1_2; break;
        case 2:  inner_fec = FEC_2_3; break;
        case 3:  inner_fec = FEC_3_4; break;
        case 4:  inner_fec = FEC_5_6; break;
        case 5:  inner_fec = FEC_7_8; break;
        case 6:  inner_fec = FEC_8_9; break;
        case 7:  inner_fec = FEC_3_5; break;
        case 8:  inner_fec = FEC_4_5; break;
        case 9:  inner_fec = FEC_9_10; break;
        case 15: inner_fec = FEC_NONE; break;
        default: inner_fec = FEC_AUTO; break;
    }

    // Modulation type.
    switch (data[6] & 0x03) {
        case 0: modulation = QAM_AUTO; break;
        case 1: modulation = QPSK; break;
        case 2: modulation = PSK_8; break;
        case 3: modulation = QAM_16; break;
        default: assert(false);
    }

    // Modulation system.
    switch ((data[6] >> 2) & 0x01) {
        case 0:
            delivery_system = DS_DVB_S;
            roll_off = ROLLOFF_AUTO;
            break;
        case 1:
            delivery_system = DS_DVB_S2;
            // Roll off.
            switch ((data[6] >> 3) & 0x03) {
                case 0: roll_off = ROLLOFF_35; break;
                case 1: roll_off = ROLLOFF_25; break;
                case 2: roll_off = ROLLOFF_20; break;
                case 3: roll_off = ROLLOFF_AUTO; break;
                default: assert(false);
            }
            break;
        default:
            assert(false);
    }

    return true;
}


//----------------------------------------------------------------------------
// Convert to and from XML.
//----------------------------------------------------------------------------

ts::xml::Element* ts::TunerParametersDVBS::toXML(xml::Element* parent) const
{
    xml::Element* e = parent->addElement(u"dvbs");
    if (satellite_number != 0) {
        e->setIntAttribute(u"satellite", satellite_number, false);
    }
    e->setIntAttribute(u"frequency", frequency, false);
    e->setIntAttribute(u"symbolrate", symbol_rate, false);
    e->setEnumAttribute(ModulationEnum, u"modulation", modulation);
    if (delivery_system != DS_DVB_S) {
        e->setEnumAttribute(DeliverySystemEnum, u"system", delivery_system);
    }
    if (polarity != POL_AUTO) {
        e->setEnumAttribute(PolarizationEnum, u"polarity", polarity);
    }
    if (inversion != SPINV_AUTO) {
        e->setEnumAttribute(SpectralInversionEnum, u"inversion", inversion);
    }
    if (inner_fec != FEC_AUTO) {
        e->setEnumAttribute(InnerFECEnum, u"FEC", inner_fec);
    }
    if (delivery_system == DS_DVB_S2 && pilots != PILOT_AUTO) {
        e->setEnumAttribute(PilotEnum, u"pilots", pilots);
    }
    if (delivery_system == DS_DVB_S2 && roll_off != ROLLOFF_AUTO) {
        e->setEnumAttribute(RollOffEnum, u"rolloff", roll_off);
    }
    return e;
}

bool ts::TunerParametersDVBS::fromXML(const xml::Element* elem)
{
    return elem != nullptr &&
        elem->name() == u"dvbs" &&
        elem->getIntAttribute<size_t>(satellite_number, u"satellite", false, 0, 0, 3) &&
        elem->getIntAttribute<uint64_t>(frequency, u"frequency", true) &&
        elem->getIntAttribute<uint32_t>(symbol_rate, u"symbolrate", false, 27500000) &&
        elem->getIntEnumAttribute(modulation, ModulationEnum, u"modulation", false, QPSK) &&
        elem->getIntEnumAttribute(delivery_system, DeliverySystemEnum, u"system", false, DS_DVB_S) &&
        elem->getIntEnumAttribute(inner_fec, InnerFECEnum, u"FEC", false, FEC_AUTO) &&
        elem->getIntEnumAttribute(inversion, SpectralInversionEnum, u"inversion", false, SPINV_AUTO) &&
        elem->getIntEnumAttribute(polarity, PolarizationEnum, u"polarity", false, POL_AUTO) &&
        (delivery_system == DS_DVB_S || elem->getIntEnumAttribute(pilots, PilotEnum, u"pilots", false, PILOT_AUTO)) &&
        (delivery_system == DS_DVB_S || elem->getIntEnumAttribute(roll_off, RollOffEnum, u"rolloff", false, ROLLOFF_AUTO));
}


//----------------------------------------------------------------------------
// This abstract method computes the theoretical useful bitrate of a
// transponder, based on 188-bytes packets, in bits/second.
// If the characteristics of the transponder are not sufficient
// to compute the bitrate, return 0.
//----------------------------------------------------------------------------

ts::BitRate ts::TunerParametersDVBS::theoreticalBitrate() const
{
    // Let the Dektec API compute the TS rate if we have a Dektec library.
#if !defined(TS_NO_DTAPI)
    int bitrate, modulation_type, param0, param1, param2;
    if (convertToDektecModulation(modulation_type, param0, param1, param2) &&
        Dtapi::DtapiModPars2TsRate(bitrate, modulation_type, param0, param1, param2, int(symbol_rate)) == DTAPI_OK) {
        // Successfully found Dektec modulation parameters and computed TS bitrate
        return BitRate(bitrate);
    }
#endif

    // Otherwise, don't know how to compute DVB-S2 bitrate...
    return delivery_system == DS_DVB_S ? TheoreticalBitrateForModulation(modulation, inner_fec, symbol_rate) : 0;
}


//----------------------------------------------------------------------------
// Attempt to convert the tuning parameters in modulation parameters
// Dektec modulator cards. This is an optional method. Return true
// on success, false on error (includes unsupported operation).
//----------------------------------------------------------------------------

bool ts::TunerParametersDVBS::convertToDektecModulation (int& modulation_type, int& param0, int& param1, int& param2) const
{
#if defined(TS_NO_DTAPI)

    // No Dektec library.
    return false;

#else

    // Determine modulation type
    if (delivery_system == DS_DVB_S) {
        modulation_type = DTAPI_MOD_DVBS_QPSK;
    }
    else if (delivery_system == DS_DVB_S2 && modulation == QPSK) {
        modulation_type = DTAPI_MOD_DVBS2_QPSK;
    }
    else if (delivery_system == DS_DVB_S2 && modulation == PSK_8) {
        modulation_type = DTAPI_MOD_DVBS2_8PSK;
    }
    else {
        return false; // unsupported
    }

    // Determine convolution code rate
    switch (inner_fec) {
        case FEC_1_2:  param0 = DTAPI_MOD_1_2; break;
        case FEC_1_3:  param0 = DTAPI_MOD_1_3; break;
        case FEC_1_4:  param0 = DTAPI_MOD_1_4; break;
        case FEC_2_3:  param0 = DTAPI_MOD_2_3; break;
        case FEC_2_5:  param0 = DTAPI_MOD_2_5; break;
        case FEC_3_4:  param0 = DTAPI_MOD_3_4; break;
        case FEC_3_5:  param0 = DTAPI_MOD_3_5; break;
        case FEC_4_5:  param0 = DTAPI_MOD_4_5; break;
        case FEC_5_6:  param0 = DTAPI_MOD_5_6; break;
        case FEC_6_7:  param0 = DTAPI_MOD_6_7; break;
        case FEC_7_8:  param0 = DTAPI_MOD_7_8; break;
        case FEC_8_9:  param0 = DTAPI_MOD_8_9; break;
        case FEC_9_10: param0 = DTAPI_MOD_9_10; break;
        default: return false; // unsupported
    }

    // Additional parameters param1 and param2
    switch (delivery_system) {
        case DS_DVB_S: {
            param1 = param2 = 0;
            break;
        }
        case DS_DVB_S2: {
            switch (pilots) {
                case PILOT_ON:  param1 = DTAPI_MOD_S2_PILOTS; break;
                case PILOT_OFF: param1 = DTAPI_MOD_S2_NOPILOTS; break;
                default: return false; // unsupported
            }
            // Assume long FEC frame for broadcast service (should be updated by caller if necessary).
            param1 |= DTAPI_MOD_S2_LONGFRM;
            // No physical layer scrambling initialization sequence
            param2 = 0;
            break;
        }
        default: {
            return false; // unsupported
        }
    }

    return true;

#endif // TS_NO_DTAPI
}
