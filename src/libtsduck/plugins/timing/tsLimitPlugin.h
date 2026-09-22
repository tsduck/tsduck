//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Bitrate limiter plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsSectionDemux.h"
#include "tsBinaryTable.h"

namespace ts {
    //!
    //! Bitrate limiter plugin.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL LimitPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(LimitPlugin);
    public:
        // Implementation of plugin API.
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Context per PID in the TS.
        class PIDContext;
        using PIDContextPtr = std::shared_ptr<PIDContext>;
        using PIDContextMap = std::map<PID, PIDContextPtr>;

        // Default threshold values.
        static constexpr PacketCounter DEFAULT_THRESHOLD1 = 10;
        static constexpr PacketCounter DEFAULT_THRESHOLD2 = 100;
        static constexpr PacketCounter DEFAULT_THRESHOLD3 = 500;
        static constexpr PacketCounter DEFAULT_THRESHOLD4 = 1000;

        // Plugin fields.
        bool           _useWallClock = false;
        BitRate        _maxBitrate = 0;
        PacketCounter  _threshold1 = 0;
        PacketCounter  _threshold2 = 0;
        PacketCounter  _threshold3 = 0;
        PacketCounter  _threshold4 = 0;
        PacketCounter  _thresholdAV = 0;     // Threshold for audio/video packets.
        BitRate        _curBitrate = 0;      // Instant bitrate (between to consecutive PCR).
        PacketCounter  _excessPoint = 0;     // Last packet from which we computed excess packets.
        PacketCounter  _excessPackets = 0;   // Number of packets in excess (to drop).
        PacketCounter  _excessBits = 0;      // Number of bits in excess, in addition to packets.
        PIDSet         _pids1 {};            // PIDs to sacrifice at threshold 1.
        SectionDemux   _demux {duck, this};  // Demux to collect PAT and PMT's.
        PIDContextMap  _pidContexts {};      // One context per PID in the TS.
        monotonic_time _clock {};            // Monotonic clock for live streams.
        size_t         _bitsSecond = 0;      // Number of bits in current second.

        // Context per PID in the TS.
        class PIDContext
        {
            TS_NOBUILD_NOCOPY(PIDContext);
        public:
            PIDContext(PID pid);                  // Constructor.
            const PID     pid;                    // PID value.
            bool          psi = false;            // The PID contains PSI/SI.
            bool          video = false;          // The PID contains video.
            bool          audio = false;          // The PID contains audio.
            uint64_t      pcrValue = INVALID_PCR; // Last PCR value.
            PacketCounter pcrPacket = 0;          // Global packet index for pcrValue.
            PacketCounter dropCount = 0;          // Number of dropped packets in this PID.
        };

        // Get or create the context for a PID.
        PIDContextPtr getContext(PID pid);

        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux& demux, const BinaryTable& table) override;

        // Add bits in excess in counters.
        void addExcessBits(uint64_t bits);
    };
}
