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
//  DVB-C (cable, QAM) tuners parameters
//
//----------------------------------------------------------------------------

#include "tsTunerParametersDVBC.h"
#include "tsTunerArgs.h"
#include "tsEnumeration.h"
#include "tsDecimal.h"
#include "tsFormat.h"

#if defined (TS_NEED_STATIC_CONST_DEFINITIONS)
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

void ts::TunerParametersDVBC::copy (const TunerParameters& obj) throw (IncompatibleTunerParametersError)
{
    const TunerParametersDVBC* other = dynamic_cast <const TunerParametersDVBC*> (&obj);
    if (other == 0) {
        throw IncompatibleTunerParametersError ("DVBC != " + TunerTypeEnum.name (obj.tunerType()));
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
// Values as encoded in zap format
//----------------------------------------------------------------------------

namespace {

    const ts::Enumeration ZapModulationEnum
        ("QPSK",     ts::QPSK,
         "QAM_AUTO", ts::QAM_AUTO,
         "QAM_16",   ts::QAM_16,
         "QAM_32",   ts::QAM_32,
         "QAM_64",   ts::QAM_64,
         "QAM_128",  ts::QAM_128,
         "QAM_256",  ts::QAM_256,
         TS_NULL);

    const ts::Enumeration ZapSpectralInversionEnum
        ("INVERSION_OFF",  ts::SPINV_OFF,
         "INVERSION_ON",   ts::SPINV_ON,
         "INVERSION_AUTO", ts::SPINV_AUTO,
         TS_NULL);

    const ts::Enumeration ZapInnerFECEnum
        ("FEC_NONE", ts::FEC_NONE,
         "FEC_AUTO", ts::FEC_AUTO,
         "FEC_1/2",  ts::FEC_1_2,
         "FEC_2/3",  ts::FEC_2_3,
         "FEC_3/4",  ts::FEC_3_4,
         "FEC_4/5",  ts::FEC_4_5,
         "FEC_5/6",  ts::FEC_5_6,
         "FEC_6/7",  ts::FEC_6_7,
         "FEC_7/8",  ts::FEC_7_8,
         "FEC_8/9",  ts::FEC_8_9,
         TS_NULL);
}


//----------------------------------------------------------------------------
// Format the tuner parameters according to the Linux DVB "zap" format
// Expected format: "freq:inv:symrate:conv:mod"
//    With freq = frequency in Hz, inv = inversion (one of INVERSION_OFF,
//    INVERSION_ON, INVERSION_AUTO), symrate = symbol rate in sym/s, conv =
//    convolutional rate (one of FEC_NONE, FEC_1_2, FEC_2_3, FEC_3_4,
//    FEC_4_5, FEC_5_6, FEC_6_7, FEC_7_8, FEC_8_9, FEC_AUTO), mod = modulation
//    (one of QPSK, QAM_16, QAM_32, QAM_64, QAM_128, QAM_256, QAM_AUTO).
//----------------------------------------------------------------------------

std::string ts::TunerParametersDVBC::toZapFormat() const
{
    return Format ("%" FMT_INT64 "u:", frequency) +
        ZapSpectralInversionEnum.name (inversion) +
        Format (":%d:", int (symbol_rate)) +
        ZapInnerFECEnum.name (inner_fec) + ":" +
        ZapModulationEnum.name (modulation);
}


//----------------------------------------------------------------------------
// Decode a Linux DVB "zap" specification and set the corresponding values
//----------------------------------------------------------------------------

bool ts::TunerParametersDVBC::fromZapFormat (const std::string& zap)
{
    StringVector values;
    SplitString (values, zap, ':', true);

    uint64_t freq;
    int inv;
    uint32_t symrate;
    int fec;
    int mod;

    if (values.size() != 5 ||
        !ToInteger (freq, values[0]) ||
        (inv = ZapSpectralInversionEnum.value (values[1])) == Enumeration::UNKNOWN ||
        !ToInteger (symrate, values[2]) ||
        (fec = ZapInnerFECEnum.value (values[3])) == Enumeration::UNKNOWN ||
        (mod = ZapModulationEnum.value (values[4])) == Enumeration::UNKNOWN) {
        return false;
    }

    frequency = freq;
    inversion = SpectralInversion (inv);
    symbol_rate = symrate;
    inner_fec = InnerFEC (fec);
    modulation = Modulation (mod);

    return true;
}


//----------------------------------------------------------------------------
// Format the tuner parameters as a list of options for the dvb tsp plugin.
//----------------------------------------------------------------------------

std::string ts::TunerParametersDVBC::toPluginOptions(bool no_local) const
{
    return Format("--frequency %" FMT_INT64 "u --symbol-rate %d", frequency, int(symbol_rate)) +
        " --fec-inner " + InnerFECEnum.name(inner_fec) +
        " --spectral-inversion " + SpectralInversionEnum.name(inversion) +
        " --modulation " + ModulationEnum.name(modulation);
}


//----------------------------------------------------------------------------
// Format a short description (frequency and essential parameters).
//----------------------------------------------------------------------------

std::string ts::TunerParametersDVBC::shortDescription(int strength, int quality) const
{
    std::string desc(Decimal(frequency) + " Hz, " + ModulationEnum.name(modulation));
    if (strength >= 0) {
        desc += Format(", strength: %d%%", strength);
    }
    if (quality >= 0) {
        desc += Format(", quality: %d%%", quality);
    }
    return desc;
}


//----------------------------------------------------------------------------
// Display a description of the modulation paramters on a stream, line by line.
//----------------------------------------------------------------------------

void ts::TunerParametersDVBC::displayParameters(std::ostream& strm, const std::string& margin, bool verbose) const
{
    if (frequency != 0) {
        strm << margin << "Carrier frequency: " << Decimal(frequency) << " Hz" << std::endl;
    }
    if (inversion != SPINV_AUTO) {
        strm << margin << "Spectral inversion: " << SpectralInversionEnum.name(inversion) << std::endl;
    }
    if (symbol_rate != 0) {
        strm << margin << "Symbol rate: " << ts::Decimal(symbol_rate) << " symb/s" << std::endl;
    }
    if (inner_fec != FEC_AUTO) {
        strm << margin << "FEC inner: " << ts::InnerFECEnum.name(inner_fec) << std::endl;
    }
    if (modulation != QAM_AUTO) {
        strm << margin << "Modulation: " << ModulationEnum.name(modulation) << std::endl;
    }
}


//----------------------------------------------------------------------------
// Extract options from a TunerArgs, applying defaults when necessary.
//----------------------------------------------------------------------------

bool ts::TunerParametersDVBC::fromArgs (const TunerArgs& tuner, ReportInterface& report)
{
    if (!tuner.frequency.set()) {
        report.error ("no frequency specified, use option --frequency");
        return false;
    }

    frequency = tuner.frequency.value();
    symbol_rate = tuner.symbol_rate.set() ? tuner.symbol_rate.value() : DEFAULT_SYMBOL_RATE;
    inner_fec = tuner.inner_fec.set() ? tuner.inner_fec.value() : DEFAULT_INNER_FEC;
    inversion = tuner.inversion.set() ? tuner.inversion.value() : DEFAULT_INVERSION;
    modulation = tuner.modulation.set() ? tuner.modulation.value() : DEFAULT_MODULATION;

    return true;
}
