//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Modulation parameters for tuners and their command-line definitions.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsObject.h"
#include "tsDescriptorList.h"
#include "tsDeliverySystem.h"
#include "tsDisplayInterface.h"
#include "tsModulation.h"
#include "tsBitRate.h"
#include "tsLNB.h"
#include "tsUnicable.h"
#include "tsjson.h"

namespace ts {

    class Args;
    class DuckContext;
    class Descriptor;
    class ModulationArgs;

    //!
    //! Safe pointer for ModulationArgs (thread-safe).
    //!
    using ModulationArgsPtr = std::shared_ptr<ModulationArgs>;

    //!
    //! Modulation parameters for tuners and their command-line definitions.
    //! @ingroup libtsduck hardware
    //!
    //! All values may be "set" or "unset", depending on command line arguments.
    //! All options for all types of tuners are included here.
    //!
    class TSDUCKDLL ModulationArgs : public Object, public DisplayInterface
    {
    public:
        //!
        //! Delivery system (DS_DVB_*).
        //! Applies to all tuners. When unset, the default delivery system for that tuner is used.
        //!
        std::optional<DeliverySystem> delivery_system {};
        //!
        //! Frequency in Hz.
        //! Applies to all tuners. This is a mandatory parameter.
        //!
        std::optional<uint64_t> frequency {};
        //!
        //! Polarity.
        //! Applies to: DVB-S/S2, ISDB-S.
        //!
        std::optional<Polarization> polarity {};
        //!
        //! Default value for polarity.
        //!
        static constexpr Polarization DEFAULT_POLARITY = POL_VERTICAL;
        //!
        //! Local dish LNB for frequency adjustment.
        //! Applies to: DVB-S/S2, ISDB-S.
        //!
        std::optional<LNB> lnb {};
        //!
        //! Unicable switch description (for Unicable satellite distribution).
        //! Applies to: DVB-S/S2.
        //!
        std::optional<Unicable> unicable {};
        //!
        //! Spectral inversion.
        //! Applies to: DVB-T/T2, DVB-S/S2, DVB-C (A,B,C), ATSC, ISDB-T, ISDB-S.
        //!
        std::optional<SpectralInversion> inversion {};
        //!
        //! Default value for inversion.
        //!
        static constexpr SpectralInversion DEFAULT_INVERSION = SPINV_AUTO;
        //!
        //! Symbol rate in symbols / second.
        //! Applies to: DVB-S/S2, DVB-C (A,C), ISDB-S.
        //!
        std::optional<uint32_t> symbol_rate {};
        //!
        //! Default value for symbol_rate on DVB-S.
        //!
        static constexpr uint32_t DEFAULT_SYMBOL_RATE_DVBS = 27500000;
        //!
        //! Default value for symbol_rate on DVB-C.
        //!
        static constexpr uint32_t DEFAULT_SYMBOL_RATE_DVBC = 6900000;
        //!
        //! Default value for symbol_rate on ISDB-S.
        //!
        static constexpr uint32_t DEFAULT_SYMBOL_RATE_ISDBS = 28860000;
        //!
        //! Error correction.
        //! Applies to: DVB-S/S2, DVB-C (A,C), ISDB-S.
        //!
        std::optional<InnerFEC> inner_fec {};
        //!
        //! Default value for inner_fec.
        //!
        static constexpr InnerFEC DEFAULT_INNER_FEC = FEC_AUTO;
        //!
        //! Satellite index for DiSeqC switches.
        //! Applies to: DVB-S/S2, ISDB-S.
        //!
        std::optional<size_t> satellite_number {};
        //!
        //! Default value for satellite_number.
        //!
        static constexpr size_t DEFAULT_SATELLITE_NUMBER = 0;
        //!
        //! Constellation or modulation type.
        //! Applies to: DVB-T/T2, DVB-S2/Turbo, DVB-C (A,B,C), ATSC.
        //!
        std::optional<Modulation> modulation {};
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
        //! Bandwidth in Hz.
        //! Applies to: DVB-T/T2, ISDB-T.
        //!
        std::optional<BandWidth> bandwidth {};
        //!
        //! Default value for bandwidth on DVB-T.
        //!
        static constexpr BandWidth DEFAULT_BANDWIDTH_DVBT = 8000000;
        //!
        //! Default value for bandwidth on ISDB-T.
        //!
        static constexpr BandWidth DEFAULT_BANDWIDTH_ISDBT = 6000000;
        //!
        //! High priority stream code rate.
        //! Applies to: DVB-T/T2.
        //!
        std::optional<InnerFEC> fec_hp {};
        //!
        //! Default value for fec_hp.
        //!
        static constexpr InnerFEC DEFAULT_FEC_HP = FEC_AUTO;
        //!
        //! Low priority stream code rate.
        //! Applies to: DVB-T/T2.
        //!
        std::optional<InnerFEC> fec_lp {};
        //!
        //! Default value for fec_lp.
        //!
        static constexpr InnerFEC DEFAULT_FEC_LP = FEC_AUTO;
        //!
        //! Transmission mode.
        //! Applies to: DVB-T/T2, ISDB-T.
        //!
        std::optional<TransmissionMode> transmission_mode {};
        //!
        //! Default value for transmission_mode on DVB-T.
        //!
        static constexpr TransmissionMode DEFAULT_TRANSMISSION_MODE_DVBT = TM_8K;
        //!
        //! Default value for transmission_mode on ISDB-T.
        //!
        static constexpr TransmissionMode DEFAULT_TRANSMISSION_MODE_ISDBT = TM_8K;
        //!
        //! Guard interval.
        //! Applies to: DVB-T/T2, ISDB-T.
        //!
        std::optional<GuardInterval> guard_interval {};
        //!
        //! Default value for guard_interval on DVB-T.
        //!
        static constexpr GuardInterval DEFAULT_GUARD_INTERVAL_DVBT = GUARD_1_32;
        //!
        //! Default value for guard_interval on ISDB-T.
        //!
        static constexpr GuardInterval DEFAULT_GUARD_INTERVAL_ISDBT = GUARD_1_32;
        //!
        //! Hierarchy.
        //! Applies to: DVB-T/T2.
        //!
        std::optional<Hierarchy> hierarchy {};
        //!
        //! Default value for hierarchy.
        //!
        static constexpr Hierarchy DEFAULT_HIERARCHY = HIERARCHY_NONE;
        //!
        //! Presence of pilots.
        //! Applies to: DVB-S2.
        //!
        std::optional<Pilot> pilots {};
        //!
        //! Default value for pilots.
        //!
        static constexpr Pilot DEFAULT_PILOTS = PILOT_OFF;
        //!
        //! Roll-off factor.
        //! Applies to: DVB-S2.
        //!
        std::optional<RollOff> roll_off {};
        //!
        //! Default value for roll_off.
        //! Implied value in DVB-S, default for DVB-S2.
        //!
        static constexpr RollOff DEFAULT_ROLL_OFF = ROLLOFF_35;
        //!
        //! Physical Layer Pipe (PLP) identification.
        //! Applies to: DVB-T2.
        //!
        std::optional<uint32_t> plp {};
        //!
        //! Default value for PLP id.
        //!
        static constexpr uint32_t DEFAULT_PLP = PLP_DISABLE;
        //!
        //! Input Stream Id (ISI).
        //! Applies to: DVB-S2.
        //!
        std::optional<uint32_t> isi {};
        //!
        //! Default value for input stream id.
        //!
        static constexpr uint32_t DEFAULT_ISI = ISI_DISABLE;
        //!
        //! Physical Layer Scrambling (PLS) code.
        //! Applies to: DVB-S2.
        //!
        std::optional<uint32_t> pls_code {};
        //!
        //! Default value for PLS code.
        //!
        static constexpr uint32_t DEFAULT_PLS_CODE = 0;
        //!
        //! Physical Layer Scrambling (PLS) mode.
        //! Applies to: DVB-S2.
        //!
        std::optional<PLSMode> pls_mode {};
        //!
        //! Default value for PLS mode.
        //!
        static constexpr PLSMode DEFAULT_PLS_MODE = PLS_GOLD;
        //!
        //! Sound broadcasting.
        //! When specified to true, the reception is an ISDB-Tsb channel instead of an ISDB-T one.
        //! Applies to: ISDB-T.
        //!
        std::optional<bool> sound_broadcasting {};
        //!
        //! Sound broadcasting sub-channel id.
        //! When @a sound_broadcasting is specified to true, specify the sub-channel ID of the
        //! segment to be demodulated in the ISDB-Tsb channel. Possible values: 0 to 41.
        //! Applies to: ISDB-T.
        //!
        std::optional<int> sb_subchannel_id {};
        //!
        //! Default value for ISDB-Tsb sub-channel id.
        //!
        static constexpr int DEFAULT_SB_SUBCHANNEL_ID = 0;
        //!
        //! Sound broadcasting segment count.
        //! When @a sound_broadcasting is specified to true, specify the total count of connected ISDB-Tsb channels.
        //! Possible values: 1 to 13.
        //! Applies to: ISDB-T.
        //!
        std::optional<int> sb_segment_count {};
        //!
        //! Default value for ISDB-Tsb segment count.
        //!
        static constexpr int DEFAULT_SB_SEGMENT_COUNT = 13;
        //!
        //! Sound broadcasting segment index.
        //! When @a sound_broadcasting is specified to true, specify the index of the segment to be demodulated for
        //! an ISDB-Tsb channel where several of them are transmitted in the connected manner.
        //! Possible values: 0 to sb_segment_count - 1.
        //! Applies to: ISDB-T.
        //!
        std::optional<int> sb_segment_index {};
        //!
        //! Default value for ISDB-Tsb segment index.
        //!
        static constexpr int DEFAULT_SB_SEGMENT_INDEX = 0;
        //!
        //! ISDB-T hierarchical layers
        //! ISDB-T channels can be coded hierarchically. As opposed to DVB-T in ISDB-T hierarchical layers can
        //! be decoded simultaneously. For that reason a ISDB-T demodulator has 3 viterbi and 3 reed-solomon-decoders.
        //! ISDB-T has 3 hierarchical layers which each can use a part of the available segments.
        //! The total number of segments over all layers has to 13 in ISDB-T.
        //! Hierarchical reception in ISDB-T is achieved by enabling or disabling layers in the decoding process.
        //! The specified string contains a combination of characters 'A', 'B', 'C', indicating which layers
        //! shall be used.
        //! Applies to: ISDB-T.
        //!
        std::optional<UString> isdbt_layers {};
        //!
        //! Default value for ISDB-T layers (all layers: "ABC").
        //!
        static constexpr const UChar* DEFAULT_ISDBT_LAYERS = u"ABC";
        //!
        //! Check if an ISDB-T time interleaving value is valid.
        //! @param [in] ti Time interleaving value. Valid values: 0, 1, 2, 4 or -1 (auto).
        //! @return True if @a ti is valid, false otherwise.
        //!
        static bool IsValidISDBTTimeInterleaving(int ti);
        //!
        //! ISDB-T partial reception.
        //! When specified to true, the reception of the ISDB-T channel is in partial reception mode.
        //! Applies to: ISDB-T.
        //!
        std::optional<bool> isdbt_partial_reception {};
        //!
        //! Maximum value for ISDB-T segment count.
        //!
        static constexpr int MAX_ISDBT_SEGMENT_COUNT = 13;
        //!
        //! Layer A code rate.
        //! Must be one of FEC_AUTO, FEC_1_2, FEC_2_3, FEC_3_4, FEC_5_6, FEC_7_8. The default is automatically detected.
        //! Applies to: ISDB-T.
        //!
        std::optional<InnerFEC> layer_a_fec {};
        //!
        //! Layer A modulation.
        //! Must be one of QAM_AUTO, QPSK, QAM_16, QAM_64, DQPSK. The default is automatically detected.
        //! Applies to: ISDB-T.
        //!
        std::optional<Modulation> layer_a_modulation {};
        //!
        //! Layer A segment count.
        //! Possible values: 0 to 13 or -1 (auto). The default is automatically detected.
        //! Applies to: ISDB-T.
        //!
        std::optional<int> layer_a_segment_count {};
        //!
        //! Layer A time interleaving.
        //! Possible values: 0, 1, 2, 4 or -1 (auto). The default is automatically detected.
        //! Applies to: ISDB-T.
        //!
        std::optional<int> layer_a_time_interleaving {};
        //!
        //! Layer B code rate.
        //! Must be one of FEC_AUTO, FEC_1_2, FEC_2_3, FEC_3_4, FEC_5_6, FEC_7_8. The default is automatically detected.
        //! Applies to: ISDB-T.
        //!
        std::optional<InnerFEC> layer_b_fec {};
        //!
        //! Layer B modulation.
        //! Must be one of QAM_AUTO, QPSK, QAM_16, QAM_64, DQPSK. The default is automatically detected.
        //! Applies to: ISDB-T.
        //!
        std::optional<Modulation> layer_b_modulation {};
        //!
        //! Layer B segment count.
        //! Possible values: 0 to 13 or -1 (auto). The default is automatically detected.
        //! Applies to: ISDB-T.
        //!
        std::optional<int> layer_b_segment_count {};
        //!
        //! Layer B time interleaving.
        //! Possible values: 0, 1, 2, 4 or -1 (auto). The default is automatically detected.
        //! Applies to: ISDB-T.
        //!
        std::optional<int> layer_b_time_interleaving {};
        //!
        //! Layer C code rate.
        //! Must be one of FEC_AUTO, FEC_1_2, FEC_2_3, FEC_3_4, FEC_5_6, FEC_7_8. The default is automatically detected.
        //! Applies to: ISDB-T.
        //!
        std::optional<InnerFEC> layer_c_fec {};
        //!
        //! Layer C modulation.
        //! Must be one of QAM_AUTO, QPSK, QAM_16, QAM_64, DQPSK. The default is automatically detected.
        //! Applies to: ISDB-T.
        //!
        std::optional<Modulation> layer_c_modulation {};
        //!
        //! Layer C segment count.
        //! Possible values: 0 to 13 or -1 (auto). The default is automatically detected.
        //! Applies to: ISDB-T.
        //!
        std::optional<int> layer_c_segment_count {};
        //!
        //! Layer C time interleaving.
        //! Possible values: 0, 1, 2, 4 or -1 (auto). The default is automatically detected.
        //! Applies to: ISDB-T.
        //!
        std::optional<int> layer_c_time_interleaving {};
        //!
        //! Inner stream id (same as inner transport stream id).
        //! Applies to: ISDB-S (multi-stream).
        //!
        std::optional<uint32_t> stream_id {};
        //!
        //! Default value for inner stream id.
        //!
        static constexpr uint32_t DEFAULT_STREAM_ID = STREAM_ID_DISABLE;

        //!
        //! Default constructor.
        //!
        ModulationArgs() = default;

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //! @param [in] allow_short_options If true, allow short one-letter options.
        //!
        virtual void defineArgs(Args& args, bool allow_short_options);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        virtual bool loadArgs(DuckContext& duck, Args& args);

        // Implementation of DisplayInterface.
        virtual std::ostream& display(std::ostream& strm, const UString& margin = UString(), int level = Severity::Info) const override;

        //!
        //! Clear content, reset all values, they become "unset"
        //!
        virtual void clear();

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
        //! Reset the local reception parameters, they become "unset"
        //! The "local reception parameters" configure the receiving equipment (typically the dish)
        //! without affecting the received carrier signal.
        //!
        void resetLocalReceptionParameters();

        //!
        //! Copy the local reception parameters from another instance.
        //! The "local reception parameters" configure the receiving equipment (typically the dish)
        //! without affecting the received carrier signal.
        //! @param [in] other Another instance from which the local reception parameters are copied.
        //! Local reception parameters which are not set in @^a other are left unmodified in this instance.
        //!
        void copyLocalReceptionParameters(const ModulationArgs& other);

        //!
        //! Theoretical bitrate computation.
        //! All registered BitRateCalculator functions are called until one can compute the bitrate.
        //! @return The theoretical useful bitrate of a transponder, based on 188-bytes packets,
        //! in bits/second. If the characteristics of the transponder are not sufficient to compute
        //! the bitrate, return 0.
        //! @see BitRateCalculator
        //!
        BitRate theoreticalBitrate() const;

        //!
        //! Fill modulation parameters from a delivery system descriptor.
        //! This method only sets the modulation parameters from the descriptor.
        //! Other parameters are unchanged.
        //! This method can be invoked several times, on each descriptor of a descriptor list,
        //! to grab incremental values for DVB-S2 for instance.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] desc A descriptor. Must be a valid delivery system descriptor.
        //! @param [in] ts_id Tranport stream id of the TS which is described by the delivery system descriptor.
        //! @param [in] delsys Optional delivery system of the TS from which the descriptor was extracted.
        //! In the case of cable_delivery_system_descriptor, it helps to characterize the target delivery system.
        //! @return True on success, false if the descriptor was not correctly analyzed or is not
        //! a delivery system descriptor.
        //!
        bool fromDeliveryDescriptor(DuckContext& duck, const Descriptor& desc, uint16_t ts_id, DeliverySystem delsys = DS_UNDEFINED);

        //!
        //! Fill modulation parameters from delivery system descriptors in a descriptor list.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] dlist A descriptor list.
        //! @param [in] ts_id Tranport stream id of the TS which is described by the delivery system descriptor.
        //! @param [in] delsys Optional delivery system of the TS from which the descriptor list was extracted.
        //! In the case of cable_delivery_system_descriptor, it helps to characterize the target delivery system.
        //! @return True on success, false if no delivery system descriptor was found.
        //!
        bool fromDeliveryDescriptors(DuckContext& duck, const DescriptorList& dlist, uint16_t ts_id, DeliverySystem delsys = DS_UNDEFINED);

        //!
        //! Format a short description (frequency and essential parameters).
        //! @param [out] duck TSDuck execution context.
        //! @return A description string.
        //!
        UString shortDescription(DuckContext& duck) const;

        //!
        //! Format the modulation parameters as command line arguments.
        //! @param [in] no_local When true, the "local" options are not included.
        //! The local options are related to the local equipment (--lnb for instance)
        //! and may vary from one system to another for the same transponder.
        //! @return A string containing a command line options for the "dvb" tsp plugin.
        //!
        UString toPluginOptions(bool no_local = false) const;

        //!
        //! Format the modulation parameters in a JSON object.
        //! @param [in,out] obj JSON object into which the modulation parameters are added.
        //!
        void toJSON(json::Object& obj) const;

        //!
        //! Function profile to calculate a bitrate from a set modulation arguments.
        //! Calculating a bitrate can be complicated and delegated to another library.
        //! Therefore, it is possible to register various BitRateCalculator functions.
        //! @param [out] bitrate Computed bitrate.
        //! @param [in] args Modulation arguments.
        //! @return True on success, false if the function doesn't know how to calculate the bitrate.
        //!
        using BitRateCalculator = bool (*)(BitRate& bitrate, const ModulationArgs& args);

        //!
        //! A class to register BitRateCalculator functions.
        //! The registration is performed using constructors.
        //! Thus, it is possible to perform a registration in a source file, outside any code.
        //! @see TS_REGISTER_BITRATE_CALCULATOR
        //!
        class TSDUCKDLL RegisterBitRateCalculator
        {
            TS_NOBUILD_NOCOPY(RegisterBitRateCalculator);
        public:
            //!
            //! The constructor registers a new BitRateCalculator function.
            //! @param [in] func The function to calculate bitrates.
            //! @param [in] systems Delivery systems for which this function can calculate bitrates.
            //! If empty, the function is called for all delivery systems.
            //!
            RegisterBitRateCalculator(BitRateCalculator func, const DeliverySystemSet& systems);
        };

    private:
        // Bitrate calculator for QPSK or QAM modulations.
        static bool GetBitRateQAM(BitRate& bitrate, const ModulationArgs& args);
        static const RegisterBitRateCalculator _GetBitRateQAM;

        // Bitrate calculator for DVB-T and DVB-T2.
        static bool GetBitRateDVBT(BitRate& bitrate, const ModulationArgs& args);
        static const RegisterBitRateCalculator _GetBitRateDVBT;

        // Bitrate calculator for ATSC.
        static bool GetBitRateATSC(BitRate& bitrate, const ModulationArgs& args);
        static const RegisterBitRateCalculator _GetBitRateATSC;

        // Generic bitrate calculators, for all types of delivery systems.
        using GenericCalculatorsData = std::set<BitRateCalculator>;
        static GenericCalculatorsData& GenericCalculators();

        // Specialized bitrate calculators, for given types of delivery systems.
        using SpecializedCalculatorsData = std::multimap<DeliverySystem, BitRateCalculator>;
        static SpecializedCalculatorsData& SpecializedCalculators();
    };
}

//!
//! @hideinitializer
//! Registration of a new BitRateCalculator function.
//! @ingroup hardware
//! @param [in] func The function to calculate bitrates.
//! @param [in] systems Delivery systems for which this function can calculate bitrates.
//!
#define TS_REGISTER_BITRATE_CALCULATOR(func, systems) \
    [[maybe_unused]] static ts::ModulationArgs::RegisterBitRateCalculator TS_UNIQUE_NAME(_Registrar)(func, systems)
