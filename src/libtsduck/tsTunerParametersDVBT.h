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
//!
//!  @file
//!  DVB-T (terrestrial, OFDM) tuners parameters
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTunerParameters.h"

namespace ts {

    class TSDUCKDLL TunerParametersDVBT: public TunerParameters
    {
    public:
        // Public fields
        uint64_t            frequency;          // Carrier frequency, in Hz.
        SpectralInversion inversion;          // Spectral inversion, should be SPINV_AUTO.
        BandWidth         bandwidth;
        InnerFEC          fec_hp;             // High priority stream code rate
        InnerFEC          fec_lp;             // Low priority stream code rate
        Modulation        modulation;         // Constellation (modulation type)
        TransmissionMode  transmission_mode;
        GuardInterval     guard_interval;
        Hierarchy         hierarchy;

        // Default values
        static const SpectralInversion DEFAULT_INVERSION = SPINV_AUTO;
        static const BandWidth DEFAULT_BANDWIDTH = BW_8_MHZ;
        static const InnerFEC DEFAULT_FEC_HP = FEC_AUTO;
        static const InnerFEC DEFAULT_FEC_LP = FEC_AUTO;
        static const Modulation DEFAULT_MODULATION = QAM_64;
        static const TransmissionMode DEFAULT_TRANSMISSION_MODE = TM_8K;
        static const GuardInterval DEFAULT_GUARD_INTERVAL = GUARD_1_32;
        static const Hierarchy DEFAULT_HIERARCHY = HIERARCHY_NONE;

        // Constructor
        TunerParametersDVBT();

        // Implementation of TunerParameters API
        virtual BitRate theoreticalBitrate() const;
        virtual std::string toZapFormat() const;
        virtual std::string toPluginOptions (bool no_local = false) const;
        virtual bool fromZapFormat (const std::string& zap);
        virtual size_t zapFieldCount () const {return 9;}
        virtual void copy (const TunerParameters&) throw (IncompatibleTunerParametersError);
    protected:
        virtual bool fromArgs (const TunerArgs&, ReportInterface&);
    };
}
