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
//  ATSC (terrestrial, cable) tuners parameters
//
//----------------------------------------------------------------------------

#include "tsTunerParametersATSC.h"
#include "tsTunerArgs.h"
#include "tsDecimal.h"
#include "tsFormat.h"
TSDUCK_SOURCE;

#if defined (TS_NEED_STATIC_CONST_DEFINITIONS)
const ts::SpectralInversion ts::TunerParametersATSC::DEFAULT_INVERSION;
const ts::Modulation ts::TunerParametersATSC::DEFAULT_MODULATION;
#endif


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TunerParametersATSC::TunerParametersATSC() :
    TunerParameters (ATSC),
    frequency (0),
    inversion (DEFAULT_INVERSION),
    modulation (DEFAULT_MODULATION)
{
}


//----------------------------------------------------------------------------
// Virtual assignment
//----------------------------------------------------------------------------

void ts::TunerParametersATSC::copy (const TunerParameters& obj)
{
    const TunerParametersATSC* other = dynamic_cast <const TunerParametersATSC*> (&obj);
    if (other == 0) {
        throw IncompatibleTunerParametersError ("ATSC != " + TunerTypeEnum.name (obj.tunerType()));
    }
    else {
        this->frequency = other->frequency;
        this->inversion = other->inversion;
        this->modulation = other->modulation;
    }
}


//----------------------------------------------------------------------------
// Format the tuner parameters as a list of options for the dvb tsp plugin.
//----------------------------------------------------------------------------

std::string ts::TunerParametersATSC::toPluginOptions(bool no_local) const
{
    return Format("--frequency %" FMT_INT64 "u", frequency) +
        " --modulation " + ModulationEnum.name(modulation) +
        " --spectral-inversion " + SpectralInversionEnum.name(inversion);
}


//----------------------------------------------------------------------------
// Format a short description (frequency and essential parameters).
//----------------------------------------------------------------------------

std::string ts::TunerParametersATSC::shortDescription(int strength, int quality) const
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

void ts::TunerParametersATSC::displayParameters(std::ostream& strm, const std::string& margin, bool verbose) const
{
    if (frequency != 0) {
        strm << margin << "Carrier frequency: " << Decimal(frequency) << " Hz" << std::endl;
    }
    if (inversion != SPINV_AUTO) {
        strm << margin << "Spectral inversion: " << SpectralInversionEnum.name(inversion) << std::endl;
    }
    if (modulation != QAM_AUTO) {
        strm << margin << "Modulation: " << ModulationEnum.name(modulation) << std::endl;
    }
}


//----------------------------------------------------------------------------
// Extract options from a TunerArgs, applying defaults when necessary.
//----------------------------------------------------------------------------

bool ts::TunerParametersATSC::fromArgs(const TunerArgs& tuner, ReportInterface& report)
{
    if (!tuner.frequency.set()) {
        report.error ("no frequency specified, use option --frequency");
        return false;
    }

    frequency = tuner.frequency.value();
    inversion = tuner.inversion.set() ? tuner.inversion.value() : DEFAULT_INVERSION;
    modulation = tuner.modulation.set() ? tuner.modulation.value() : DEFAULT_MODULATION;

    return true;
}
