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
//!
//!  @file
//!  DVB-T (terrestrial, OFDM) tuners parameters
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTunerParameters.h"

namespace ts {
    //!
    //! DVB-T / DVB-T2 (terrestrial, OFDM) tuners parameters
    //! @ingroup hardware
    //!
    class TSDUCKDLL TunerParametersDVBT: public TunerParameters
    {
    public:
        // Public fields
        uint64_t          frequency;          //!< Carrier frequency, in Hz.
        SpectralInversion inversion;          //!< Spectral inversion, should be SPINV_AUTO.
        BandWidth         bandwidth;          //!< Bandwidth.
        InnerFEC          fec_hp;             //!< High priority stream code rate.
        InnerFEC          fec_lp;             //!< Low priority stream code rate.
        Modulation        modulation;         //!< Constellation (modulation type).
        TransmissionMode  transmission_mode;  //!< Transmission mode.
        GuardInterval     guard_interval;     //!< Guard interval.
        Hierarchy         hierarchy;          //!< Hierarchy.
        PLP               plp;                //!< Physical Layer Pipe (PLP) id (DVB-T2 only).

        // Default values
        static const SpectralInversion DEFAULT_INVERSION         = SPINV_AUTO;      //!< Default value for inversion.
        static const BandWidth         DEFAULT_BANDWIDTH         = BW_8_MHZ;        //!< Default value for bandwidth.
        static const InnerFEC          DEFAULT_FEC_HP            = FEC_AUTO;        //!< Default value for fec_hp.
        static const InnerFEC          DEFAULT_FEC_LP            = FEC_AUTO;        //!< Default value for fec_lp.
        static const Modulation        DEFAULT_MODULATION        = QAM_64;          //!< Default value for modulation.
        static const TransmissionMode  DEFAULT_TRANSMISSION_MODE = TM_8K;           //!< Default value for transmission_mode.
        static const GuardInterval     DEFAULT_GUARD_INTERVAL    = GUARD_1_32;      //!< Default value for guard_interval.
        static const Hierarchy         DEFAULT_HIERARCHY         = HIERARCHY_NONE;  //!< Default value for hierarchy.
        static const PLP               DEFAULT_PLP               = PLP_DISABLE;     //!< Default value for PLP id.

        //!
        //! Default constructor.
        //!
        TunerParametersDVBT();

        // Implementation of TunerParameters API
        virtual BitRate theoreticalBitrate() const override;
        virtual UString shortDescription(int strength = -1, int quality = -1) const override;
        virtual UString toPluginOptions(bool no_local = false) const override;
        virtual void displayParameters(std::ostream& strm, const UString& margin = UString(), bool verbose = false) const override;
        virtual void copy(const TunerParameters&) override;

    protected:
        virtual bool fromArgs(const TunerArgs&, Report&) override;
        virtual bool fromDeliveryDescriptor(const Descriptor& desc) override;
    };
}
