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
//!  Modulation parameters for tuners and their command-line definitions.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgsSupplierInterface.h"
#include "tsVariable.h"
#include "tsModulation.h"
#include "tsMPEG.h"
#include "tsLNB.h"

namespace ts {

    class Descriptor;

    //!
    //! Modulation parameters for tuners and their command-line definitions.
    //! @ingroup hardware
    //!
    //! All values may be "set" or "unset", depending on command line arguments.
    //! All options for all types of tuners are included here.
    //!
    class TSDUCKDLL ModulationArgs : public ArgsSupplierInterface
    {
    public:
        //!
        //! Delivery system (DS_DVB_*).
        //! Applies to all tuners. When unset, the default delivery system for that tuner is used.
        //!
        Variable<DeliverySystem> delivery_system;
        //!
        //! Frequency in Hz.
        //! Applies to all tuners. This is a mandatory parameter.
        //!
        Variable<uint64_t> frequency;
        //!
        //! Polarity.
        //! Applies to: DVB-S/S2, ISDB-S.
        //!
        Variable<Polarization> polarity;
        //!
        //! Default value for polarity.
        //!
        static constexpr Polarization DEFAULT_POLARITY = POL_VERTICAL;
        //!
        //! Local dish LNB for frequency adjustment.
        //! Applies to: DVB-S/S2, ISDB-S.
        //!
        Variable<LNB> lnb;
        //!
        //! Default value for lnb.
        //!
        static const LNB& DEFAULT_LNB;
        //!
        //! Spectral inversion.
        //! Applies to: DVB-T/T2, DVB-S/S2, DVB-C (A,B,C), ATSC, ISDB-T, ISDB-S.
        //!
        Variable<SpectralInversion> inversion;
        //!
        //! Default value for inversion.
        //!
        static constexpr SpectralInversion DEFAULT_INVERSION = SPINV_AUTO;
        //!
        //! Symbol rate.
        //! Applies to: DVB-S/S2, DVB-C (A,C), ISDB-S.
        //!
        Variable<uint32_t> symbol_rate;
        //!
        //! Default value for symbol_rate on satellite.
        //!
        static constexpr uint32_t DEFAULT_SYMBOL_RATE_DVBS = 27500000;
        //!
        //! Default value for symbol_rate on cable.
        //!
        static constexpr uint32_t DEFAULT_SYMBOL_RATE_DVBC = 6900000;
        //!
        //! Error correction.
        //! Applies to: DVB-S/S2, DVB-C (A,C), ISDB-S.
        //!
        Variable<InnerFEC> inner_fec;
        //!
        //! Default value for inner_fec.
        //!
        static constexpr InnerFEC DEFAULT_INNER_FEC = FEC_AUTO;
        //!
        //! Satellite index for DiSeqC switches.
        //! Applies to: DVB-S/S2, ISDB-S.
        //!
        Variable<size_t> satellite_number;
        //!
        //! Default value for satellite_number.
        //!
        static constexpr size_t DEFAULT_SATELLITE_NUMBER = 0;
        //!
        //! Constellation or modulation type.
        //! Applies to: DVB-T/T2, DVB-S2/Turbo, DVB-C (A,B,C), ATSC.
        //!
        Variable<Modulation> modulation;
        //!
        //! Default value for modulation on satellite.
        //!
        static constexpr Modulation DEFAULT_MODULATION_DVBS = QPSK;
        //!
        //! Default value for modulation on terrestrial.
        //!
        static constexpr Modulation DEFAULT_MODULATION_DVBT = QAM_64;
        //!
        //! Default value for modulation on cable.
        //!
        static constexpr Modulation DEFAULT_MODULATION_DVBC = QAM_64;
        //!
        //! Default value for modulation on ATSC.
        //!
        static constexpr Modulation DEFAULT_MODULATION_ATSC = VSB_8;
        //!
        //! Bandwidth.
        //! Applies to: DVB-T/T2, ISDB-T.
        //!
        Variable<BandWidth> bandwidth;
        //!
        //! Default value for bandwidth on DVBT.
        //!
        static constexpr BandWidth DEFAULT_BANDWIDTH_DVBT = BW_8_MHZ;
        //!
        //! High priority stream code rate.
        //! Applies to: DVB-T/T2.
        //!
        Variable<InnerFEC> fec_hp;
        //!
        //! Default value for fec_hp.
        //!
        static constexpr InnerFEC DEFAULT_FEC_HP = FEC_AUTO;
        //!
        //! Low priority stream code rate.
        //! Applies to: DVB-T/T2.
        //!
        Variable<InnerFEC> fec_lp;
        //!
        //! Default value for fec_lp.
        //!
        static constexpr InnerFEC DEFAULT_FEC_LP = FEC_AUTO;
        //!
        //! Transmission mode.
        //! Applies to: DVB-T/T2, ISDB-T.
        //!
        Variable<TransmissionMode> transmission_mode;
        //!
        //! Default value for transmission_mode on DVBT.
        //!
        static constexpr TransmissionMode DEFAULT_TRANSMISSION_MODE_DVBT = TM_8K;
        //!
        //! Guard interval.
        //! Applies to: DVB-T/T2, ISDB-T.
        //!
        Variable<GuardInterval> guard_interval;
        //!
        //! Default value for guard_interval on DVBT.
        //!
        static constexpr GuardInterval DEFAULT_GUARD_INTERVAL_DVBT = GUARD_1_32;
        //!
        //! Hierarchy.
        //! Applies to: DVB-T/T2.
        //!
        Variable<Hierarchy> hierarchy;
        //!
        //! Default value for hierarchy.
        //!
        static constexpr Hierarchy DEFAULT_HIERARCHY = HIERARCHY_NONE;
        //!
        //! Presence of pilots.
        //! Applies to: DVB-S2.
        //!
        Variable<Pilot> pilots;
        //!
        //! Default value for pilots.
        //!
        static constexpr Pilot DEFAULT_PILOTS = PILOT_OFF;
        //!
        //! Roll-off factor.
        //! Applies to: DVB-S2.
        //!
        Variable<RollOff> roll_off;
        //!
        //! Default value for roll_off.
        //! Implied value in DVB-S, default for DVB-S2.
        //!
        static constexpr RollOff DEFAULT_ROLL_OFF = ROLLOFF_35;
        //!
        //! Physical Layer Pipe (PLP) identification.
        //! Applies to: DVB-T2.
        //!
        Variable<uint32_t> plp;
        //!
        //! Default value for PLP id.
        //!
        static constexpr uint32_t DEFAULT_PLP = PLP_DISABLE;
        //!
        //! Input Stream Id (ISI).
        //! Applies to: DVB-S2.
        //!
        Variable<uint32_t> isi;
        //!
        //! Default value for input stream id.
        //!
        static constexpr uint32_t DEFAULT_ISI = ISI_DISABLE;
        //!
        //! Physical Layer Scrambling (PLS) code.
        //! Applies to: DVB-S2.
        //!
        Variable<uint32_t> pls_code;
        //!
        //! Default value for PLS code.
        //!
        static constexpr uint32_t DEFAULT_PLS_CODE = 0;
        //!
        //! Physical Layer Scrambling (PLS) mode.
        //! Applies to: DVB-S2.
        //!
        Variable<PLSMode> pls_mode;
        //!
        //! Default value for PLS mode.
        //!
        static constexpr PLSMode DEFAULT_PLS_MODE = PLS_ROOT;

        //@@  Missing values for ISDB-T:
        //@@
        //@@    DTV_ISDBT_LAYER_ENABLED
        //@@    DTV_ISDBT_PARTIAL_RECEPTION
        //@@    DTV_ISDBT_SOUND_BROADCASTING
        //@@    DTV_ISDBT_SB_SUBCHANNEL_ID
        //@@    DTV_ISDBT_SB_SEGMENT_IDX
        //@@    DTV_ISDBT_SB_SEGMENT_COUNT
        //@@    DTV_ISDBT_LAYERA_FEC
        //@@    DTV_ISDBT_LAYERA_MODULATION
        //@@    DTV_ISDBT_LAYERA_SEGMENT_COUNT
        //@@    DTV_ISDBT_LAYERA_TIME_INTERLEAVING
        //@@    DTV_ISDBT_LAYERB_FEC
        //@@    DTV_ISDBT_LAYERB_MODULATION
        //@@    DTV_ISDBT_LAYERB_SEGMENT_COUNT
        //@@    DTV_ISDBT_LAYERB_TIME_INTERLEAVING
        //@@    DTV_ISDBT_LAYERC_FEC
        //@@    DTV_ISDBT_LAYERC_MODULATION
        //@@    DTV_ISDBT_LAYERC_SEGMENT_COUNT
        //@@    DTV_ISDBT_LAYERC_TIME_INTERLEAVING
        //@@
        //@@  Missing values for ISDB-S:
        //@@
        //@@    DTV_ISDBS_TS_ID

        //!
        //! Default constructor.
        //! @param [in] allow_short_options If true, allow short one-letter options.
        //!
        explicit ModulationArgs(bool allow_short_options = true);

        // Implementation of ArgsSupplierInterface.
        virtual void defineArgs(Args& args) const override;
        virtual bool loadArgs(DuckContext& duck, Args& args) override;

        //!
        //! Reset all values, they become "unset"
        //!
        virtual void reset();

        //!
        //! Check if any modulation options is set.
        //! @return True if any modulation options is set.
        //!
        bool hasModulationArgs() const;

        //!
        //! Check the validity of the delivery system or set a default one.
        //! @param [in] systems The possible delivery systems, typically from a tuner.
        //! If the delivery system is already defined, it must be in this set.
        //! If it is not defined, the first delivery system is used.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool resolveDeliverySystem(const DeliverySystemSet& systems, Report& report);

        //!
        //! Set the default values for unset parameters, according to the delivery system.
        //! Do nothing if the delivery system is unset.
        //!
        void setDefaultValues();

        //!
        //! Theoretical bitrate computation.
        //! @return The theoretical useful bitrate of a transponder, based on 188-bytes packets,
        //! in bits/second. If the characteristics of the transponder are not sufficient to compute
        //! the bitrate, return 0.
        //!
        BitRate theoreticalBitrate() const;

        //!
        //! Fill modulation parameters from a delivery system descriptor.
        //! @param [in] desc A descriptor. Must be a valid delivery system descriptor.
        //! @return True on success, false if the descriptor was not correctly analyzed or is not
        //! a delivery system descriptor.
        //!
        bool fromDeliveryDescriptor(const Descriptor& desc);

        //!
        //! Attempt to convert the tuning parameters in modulation parameters for Dektec modulator cards.
        //! @param [out] modulation_type Modulation type (DTAPI_MOD_* value).
        //! @param [out] param0 Modulation-specific paramter 0.
        //! @param [out] param1 Modulation-specific paramter 1.
        //! @param [out] param2 Modulation-specific paramter 2.
        //! @return True on success, false on error (includes unsupported operation).
        //!
        bool convertToDektecModulation(int& modulation_type, int& param0, int& param1, int& param2) const;

    protected:
        const bool _allow_short_options;

        //!
        //! Theoretical useful bitrate for QPSK or QAM modulation.
        //! This protected static method computes the theoretical useful bitrate of a
        //! transponder, based on 188-bytes packets, for QPSK or QAM modulation.
        //! @param [in] mod Modulation type.
        //! @param [in] fec Inner FEC.
        //! @param [in] symbol_rate Symbol rate.
        //! @return Theoretical useful bitrate in bits/second or zero on error.
        //!
        static BitRate TheoreticalBitrateForModulation(Modulation mod, InnerFEC fec, uint32_t symbol_rate);
    };
}
