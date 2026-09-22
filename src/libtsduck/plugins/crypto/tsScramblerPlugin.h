//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DVB-CSA, DVB-CISSA, ATIS-IDSA scrambling packet processor plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSignalizationHandlerInterface.h"
#include "tsECMGClientHandlerInterface.h"
#include "tsServiceDiscovery.h"
#include "tsECMGClient.h"
#include "tsECMGClientArgs.h"
#include "tsTSScrambling.h"
#include "tsCyclingPacketizer.h"

namespace ts {
    //!
    //! DVB-CSA, DVB-CISSA, ATIS-IDSA scrambling packet processor plugin for tsp.
    //! @ingroup libtsduck plugin
    //!
    //! Notes on crypto-period dynamics
    //! -------------------------------
    //! A crypto-period is defined using a CryptoPeriod object (private class inside
    //! ScramblerPlugin). It contains: crypto-period number, current/next CW and ECM
    //! containing these two CW.
    //!
    //! It is necessary to maintain two CryptoPeriod objects.
    //! During crypto-period N, designated as cp(N):
    //! - Scrambling is performed using CW(N).
    //! - At beginning of cp(N), if delay_start > 0, we broadcast ECM(N-1).
    //! - In middle of cp(N), we broadcast ECM(N).
    //! - At end of cp(N), if delay_start < 0, we broadcast ECM(N+1).
    //!
    //! So, during cp(N), we need cp(N-1)/cp(N), then cp(N)/cp(N+1). On a dynamic
    //! standpoint, as soon as ECM(N-1) is no longer needed, we generate cp(N+1).
    //! In asynchronous mode, there is enough time to generate ECM(N+1) while
    //! cp(N) is finishing.
    //!
    //! The transition points in the TS are:
    //! - CW change (start a new crypto-period)
    //! - ECM change (start broadcasting a new ECM, can be before or after
    //!   start of crypto-period, depending on delay_start).
    //!
    //! Entering "degraded mode"
    //! ------------------------
    //! In asynchronous mode (the default), an ECM is actually returned by the ECMG
    //! long after it has been submitted. To complete a transition CW(N) -> CW(N+1)
    //! or ECM(N) -> ECM(N+1), we check that ECM(N+1) is ready. If it is not, we
    //! enter "degraded mode". In this mode, no transition is allowed, the same CW
    //! and ECM are used until exit of the degraded mode. This can occur when an
    //! ECM takes too long to be ciphered.
    //!
    //! Exiting "degraded mode"
    //! -----------------------
    //! When in degraded mode, each time an ECM(N) packet is inserted, we check if
    //! ECM(N+1) is ready. When it is ready, we exit degraded mode. If delay_start
    //! is negative, we immediately perform an ECM transition and we recompute the
    //! time for the next CW transition. If delay_start is positive, we immediately
    //! perform a CW transition and we recompute the time for the next ECM transition.
    //!
    class TSDUCKDLL ScramblerPlugin: public ProcessorPlugin, private SignalizationHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(ScramblerPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Additional stack size for asynchronous ECM notification.
        static constexpr size_t ASYNC_HANDLER_EXTRA_STACK_SIZE = 1024 * 1024;

        // Default ECM PID bitrate.
        static constexpr int DEFAULT_ECM_BITRATE = 30'000;

        // When bitrate is unknown, use 10 ECM/s for TS @10Mb/s
        static constexpr PacketCounter DEFAULT_ECM_INTER_PACKET = 7000;

        // Description of a crypto-period.
        // Each CryptoPeriod object points to its ScramblerPlugin parent object.
        // In case of error in a CryptoPeriod object, the _abort volatile flag
        // is set in ScramblerPlugin.
        class CryptoPeriod: private ECMGClientHandlerInterface
        {
            TS_NOCOPY(CryptoPeriod);
        public:
            // Default constructor.
            CryptoPeriod() = default;

            // Initialize first crypto period.
            // Generate two randow CW and corresponding ECM.
            // ECM generation may complete asynchronously.
            void initCycle(ScramblerPlugin*, uint16_t cp_number);

            // Initialize crypto period following specified one.
            // ECM generation may complete asynchronously.
            void initNext(const CryptoPeriod&);

            // Check if ECM generation is complete (useful in asynchronous mode)
            bool ecmReady() const { return _ecm_ok; }

            // Get next ECM packet in ECM cycle (or null packet if ECM not ready).
            void getNextECMPacket(TSPacket&);

            // Initialize the scrambler with the current control word.
            bool initScramblerKey() const;

        private:
            ScramblerPlugin* _plugin = nullptr;  // Reference to scrambler plugin
            uint16_t         _cp_number = 0;     // Crypto-period number
            volatile bool    _ecm_ok = false;    // _ecm field is valid
            TSPacketVector   _ecm {};            // Packetized ECM
            size_t           _ecm_pkt_index {};  // Next ECM packet to insert in TS
            ByteBlock        _cw_current {};
            ByteBlock        _cw_next {};

            // Generate a new random CW.
            void generateCW(ByteBlock& cw);

            // Generate the ECM for a crypto-period.
            // With --synchronous, the ECM is directly generated. Otherwise,
            // the ECM will be set later, notified through private handleECM.
            void generateECM();

            // Invoked when an ECM is available, maybe in the context of an external thread.
            virtual void handleECM(const ecmgscs::ECMResponse&) override;
        };

        // ScramblerPlugin parameters, remain constant after start()
        ServiceDiscovery  _service {duck, this};        // Service description
        bool              _use_service = false;         // Scramble a service (ie. not a specific list of PID's).
        bool              _component_level = false;     // Insert CA_descriptors at component level
        bool              _scramble_audio = false;      // Scramble all audio components
        bool              _scramble_video = false;      // Scramble all video components
        bool              _scramble_subtitles = false;  // Scramble all subtitles components
        PID               _only_pid = PID_NULL;         // Only PID to scramble (part of _service streams)
        bool              _synchronous_ecmg = false;    // Synchronous ECM generation
        bool              _ignore_scrambled = false;    // Ignore packets which are already scrambled
        bool              _update_pmt = false;          // Update PMT.
        bool              _need_cp = false;             // Need to manage crypto-periods (ie. not one single fixed CW).
        bool              _need_ecm = false;            // Need to manage ECM insertion (ie. not fixed CW's).
        bool              _pre_reduce_cw = false;       // Reduce the control word before sending to the ECMG.
        cn::milliseconds  _delay_start {0};             // Delay between CP start and ECM start (can be negative)
        ByteBlock         _ca_desc_private {};          // Private data to insert in CA_descriptor
        BitRate           _ecm_bitrate = 0;             // ECM PID's bitrate
        PID               _ecm_pid = PID_NULL;          // PID for ECM
        PacketCounter     _partial_scrambling = 0;      // Do not scramble all packets if > 1
        cn::seconds       _clear_period {0};            // Clear period before scrambling commences
        ECMGClientArgs    _ecmg_args {};                // Parameters for ECMG client
        tlv::Logger       _logger {this, Severity::Debug};  // Message logger for ECMG <=> SCS protocol
        ecmgscs::Protocol      _ecmgscs {};                 // ECMG <=> SCS protocol instance.
        ecmgscs::ChannelStatus _channel_status {_ecmgscs};  // Initial response to ECMG channel_setup
        ecmgscs::StreamStatus  _stream_status {_ecmgscs};   // Initial response to ECMG stream_setup

        // ScramblerPlugin state
        volatile bool     _abort = false;               // Error (service not found, etc)
        bool              _wait_bitrate = false;        // Waiting for bitrate to start scheduling ECM and CP.
        bool              _degraded_mode = false;       // In degraded mode (see comments above)
        PacketCounter     _packet_count = 0;            // Complete TS packet counter
        PacketCounter     _scrambled_count = 0;         // Summary of scrambled packets
        PacketCounter     _partial_clear = 0;           // How many clear packets to keep clear
        PacketCounter     _pkt_clear_period = 0;        // How many packets in initial clear period
        PacketCounter     _pkt_insert_ecm = 0;          // Insertion point for next ECM packet.
        PacketCounter     _pkt_change_cw = 0;           // Transition point for next CW change
        PacketCounter     _pkt_change_ecm = 0;          // Transition point for next ECM change
        BitRate           _ts_bitrate = 0;              // Saved TS bitrate
        ECMGClient        _ecmg {_logger, _ecmgscs, ASYNC_HANDLER_EXTRA_STACK_SIZE}; // Connection with the ECMG
        uint8_t           _ecm_cc = 0;                  // Continuity counter in ECM PID.
        PIDSet            _scrambled_pids {};           // List of pids to scramble
        PIDSet            _conflict_pids {};            // List of pids to scramble with scrambled input packets
        PIDSet            _input_pids {};               // List of input pids
        CryptoPeriod      _cp[2] {};                    // Previous/current or current/next crypto-periods
        size_t            _current_cw = 0;              // Index to current CW (current crypto period)
        size_t            _current_ecm = 0;             // Index to current ECM (ECM being broadcast)
        TSScrambling      _scrambling {*this};          // Scrambler
        CyclingPacketizer _pzer_pmt {duck};             // Packetizer for modified PMT

        // Initialize ECM and CP scheduling.
        void initializeScheduling();

        // Return current/next CryptoPeriod for CW or ECM
        CryptoPeriod& currentCW()  { return _cp[_current_cw]; }
        CryptoPeriod& nextCW()     { return _cp[(_current_cw + 1) & 0x01]; }
        CryptoPeriod& currentECM() { return _cp[_current_ecm]; }
        CryptoPeriod& nextECM()    { return _cp[(_current_ecm + 1) & 0x01]; }

        // Perform CW and ECM transition
        bool changeCW();
        void changeECM();

        // Check if we are in degraded mode or if we enter degraded mode
        bool inDegradedMode();

        // Try to exit from degraded mode
        bool tryExitDegradedMode();

        // Invoked when the PMT of the service is available.
        virtual void handlePMT(const PMT&, PID) override;
    };
}
