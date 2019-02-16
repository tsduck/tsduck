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
//  DVB-C (cable, QAM) tuners parameters
//
//----------------------------------------------------------------------------

#include "tsTunerParametersDVBC.h"
#include "tsTunerArgs.h"
#include "tsEnumeration.h"
#include "tsxmlElement.h"
#include "tsBCD.h"
TSDUCK_SOURCE;

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
const ts::SpectralInversion ts::TunerParametersDVBC::DEFAULT_INVERSION;
const uint32_t ts::TunerParametersDVBC::DEFAULT_SYMBOL_RATE;
const ts::InnerFEC ts::TunerParametersDVBC::DEFAULT_INNER_FEC;
const ts::Modulation ts::TunerParametersDVBC::DEFAULT_MODULATION;
#endif


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TunerParametersDVBC::TunerParametersDVBC() :
    TunerParameters (DVB_C),
    frequency (0),
    inversion (DEFAULT_INVERSION),
    symbol_rate (DEFAULT_SYMBOL_RATE),
    inner_fec (DEFAULT_INNER_FEC),
    modulation (DEFAULT_MODULATION)
{
}


//----------------------------------------------------------------------------
// Virtual assignment
//----------------------------------------------------------------------------

void ts::TunerParametersDVBC::copy(const TunerParameters& obj)
{
    const TunerParametersDVBC* other = dynamic_cast <const TunerParametersDVBC*> (&obj);
    if (other == nullptr) {
        throw IncompatibleTunerParametersError(u"DVBC != " + TunerTypeEnum.name(obj.tunerType()));
    }
    else {
        this->frequency = other->frequency;
        this->inversion = other->inversion;
        this->symbol_rate = other->symbol_rate;
        this->inner_fec = other->inner_fec;
        this->modulation = other->modulation;
    }
}


//----------------------------------------------------------------------------
// Format the tuner parameters as a list of options for the dvb tsp plugin.
//----------------------------------------------------------------------------

ts::UString ts::TunerParametersDVBC::toPluginOptions(bool no_local) const
{
    return UString::Format(u"--frequency %d --symbol-rate %d --fec-inner %s --spectral-inversion %s --modulation %s",
                          {frequency,
                           symbol_rate,
                           InnerFECEnum.name(inner_fec),
                           SpectralInversionEnum.name(inversion),
                           ModulationEnum.name(modulation)});
}


//----------------------------------------------------------------------------
// Format a short description (frequency and essential parameters).
//----------------------------------------------------------------------------

ts::UString ts::TunerParametersDVBC::shortDescription(int strength, int quality) const
{
    UString desc(UString::Format(u"%'d Hz, %s", {frequency, ModulationEnum.name(modulation)}));
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

void ts::TunerParametersDVBC::displayParameters(std::ostream& strm, const UString& margin, bool verbose) const
{
    if (frequency != 0) {
        strm << margin << "Carrier frequency: " << UString::Decimal(frequency) << " Hz" << std::endl;
    }
    if (inversion != SPINV_AUTO) {
        strm << margin << "Spectral inversion: " << SpectralInversionEnum.name(inversion) << std::endl;
    }
    if (symbol_rate != 0) {
        strm << margin << "Symbol rate: " << UString::Decimal(symbol_rate) << " symb/s" << std::endl;
    }
    if (inner_fec != FEC_AUTO) {
        strm << margin << "FEC inner: " << InnerFECEnum.name(inner_fec) << std::endl;
    }
    if (modulation != QAM_AUTO) {
        strm << margin << "Modulation: " << ModulationEnum.name(modulation) << std::endl;
    }
}


//----------------------------------------------------------------------------
// Extract options from a TunerArgs, applying defaults when necessary.
//----------------------------------------------------------------------------

bool ts::TunerParametersDVBC::fromArgs(const TunerArgs& tuner, Report& report)
{
    if (!tuner.frequency.set()) {
        report.error(u"no frequency specified, use option --frequency");
        return false;
    }

    frequency = tuner.frequency.value();
    symbol_rate = tuner.symbol_rate.set() ? tuner.symbol_rate.value() : DEFAULT_SYMBOL_RATE;
    inner_fec = tuner.inner_fec.set() ? tuner.inner_fec.value() : DEFAULT_INNER_FEC;
    inversion = tuner.inversion.set() ? tuner.inversion.value() : DEFAULT_INVERSION;
    modulation = tuner.modulation.set() ? tuner.modulation.value() : DEFAULT_MODULATION;

    return true;
}


//----------------------------------------------------------------------------
// Extract tuning information from a delivery descriptor.
//----------------------------------------------------------------------------

bool ts::TunerParametersDVBC::fromDeliveryDescriptor(const Descriptor& desc)
{
    if (!desc.isValid() || desc.tag() != DID_CABLE_DELIVERY || desc.payloadSize() < 11) {
        return false;
    }

    const uint8_t* data = desc.payload();
    frequency = uint64_t(DecodeBCD(data, 8)) * 100;
    symbol_rate = DecodeBCD(data + 7, 7) * 100;

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

    switch (data[6]) {
        case 1:  modulation = QAM_16; break;
        case 2:  modulation = QAM_32; break;
        case 3:  modulation = QAM_64; break;
        case 4:  modulation = QAM_128; break;
        case 5:  modulation = QAM_256; break;
        default: modulation = QAM_AUTO; break;
    }

    return true;
}


//----------------------------------------------------------------------------
// Convert to and from XML.
//----------------------------------------------------------------------------

ts::xml::Element* ts::TunerParametersDVBC::toXML(xml::Element* parent) const
{
    xml::Element* e = parent->addElement(u"dvbc");
    e->setIntAttribute(u"frequency", frequency, false);
    e->setIntAttribute(u"symbolrate", symbol_rate, false);
    e->setEnumAttribute(ModulationEnum, u"modulation", modulation);
    if (inner_fec != FEC_AUTO) {
        e->setEnumAttribute(InnerFECEnum, u"FEC", inner_fec);
    }
    if (inversion != SPINV_AUTO) {
        e->setEnumAttribute(SpectralInversionEnum, u"inversion", inversion);
    }
    return e;
}

bool ts::TunerParametersDVBC::fromXML(const xml::Element* elem)
{
    return elem != nullptr &&
        elem->name().similar(u"dvbc") &&
        elem->getIntAttribute<uint64_t>(frequency, u"frequency", true) &&
        elem->getIntAttribute<uint32_t>(symbol_rate, u"symbolrate", false, 6900000) &&
        elem->getIntEnumAttribute(modulation, ModulationEnum, u"modulation", false, QAM_64) &&
        elem->getIntEnumAttribute(inner_fec, InnerFECEnum, u"FEC", false, FEC_AUTO) &&
        elem->getIntEnumAttribute(inversion, SpectralInversionEnum, u"inversion", false, SPINV_AUTO);
}


//----------------------------------------------------------------------------
// Standard computation of bitrate.
//----------------------------------------------------------------------------

ts::BitRate ts::TunerParametersDVBC::theoreticalBitrate() const
{
    return TheoreticalBitrateForModulation(modulation, inner_fec, symbol_rate);
}
