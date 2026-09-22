//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to reduce the bitrate of the TS by dropping null packets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"

namespace ts {
    //!
    //! Plugin to reduce the bitrate of the TS by dropping null packets.
    //! @ingroup libtsduck plugin
    //!
    //! Important: this plugin works in individual packet or packet window mode,
    //! depending on the command line parameters.
    //!
    class TSDUCKDLL ReducePlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(ReducePlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual size_t getPacketWindowSize() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;
        virtual size_t processPacketWindow(TSPacketWindow& win) override;

    private:
        // Default mode: target bitrate with 10,000 packets window (620 ms at 24 Mb/s, 300 ms at 50 Mb/s)
        static constexpr PacketCounter DEFAULT_PACKET_WINDOW = 10'000;

        // Last error code (to avoid reporting the same error again and again).
        enum class Error {NONE, PKT_OVERFLOW, NO_BITRATE, USE_PREVIOUS, LOW_BITRATE};

        // Command line parameters:
        BitRate          _target_bitrate = 0;   // Target bitrate to read, zero if fixed proportion is used.
        BitRate          _input_bitrate = 0;    // User-sepcified input bitrate.
        cn::milliseconds _window_ms {};         // Packet window size in milliseconds.
        PacketCounter    _window_pkts = 0;      // Packet window size in packets.
        bool             _pcr_based = false;    // Use PCR's in packet window to compute the number f packets to remove.
        PIDSet           _pcr_pids {};          // Reference PCR PID's.
        PacketCounter    _fixed_rempkt = 0;     // rempkt parameter, zero if target
        PacketCounter    _fixed_inpkt = 0;      // inpkt parameter

        // Working data:
        PacketCounter _pkt_to_remove = 0;    // Current number of packets to remove
        uint64_t      _bits_to_remove = 0;   // Current number of bits to remove
        BitRate       _previous_bitrate = 0; // Bitrate from previous packet window.
        Error         _error = Error::NONE;  // Last error code.

        // Compute bitrate in a packet window.
        BitRate computeBitRate(const TSPacketWindow& win) const;
    };
}
