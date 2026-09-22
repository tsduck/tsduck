//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to copy PCR values from a PID into another.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Plugin to copy PCR values from a PID into another (with packet distance adjustment).
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL PCRCopyPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PCRCopyPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options.
        PID           _ref_pid_arg = PID_NULL;     // Reference PCR source.
        PID           _target_pid_arg = PID_NULL;  // Target PID to alter.
        size_t        _ref_label = NPOS;           // Label which indicates the reference PID.
        size_t        _target_label = NPOS;        // Label which indicates the target PID.
        PacketCounter _every = 0;                  // Insert a PCR every N packets (if not zero).
        size_t        _max_shift = 0;              // Maximum number of bytes to shift.
        bool          _pusi = false;               // Insert a PCR in PUSI packets.

        // Working data.
        PID           _ref_pid = PID_NULL;         // Current reference PCR source.
        PID           _target_pid = PID_NULL;      // Current target PID to alter.
        PacketCounter _target_packets = 0;         // Number of packets in target PID.
        PacketCounter _ref_packet = 0;             // Packet index of last PCR in reference PID.
        uint64_t      _ref_pcr = INVALID_PCR;      // Last PCR value in reference PID.
        uint8_t       _target_cc_in = 0;           // Last read continuity counter in target PID.
        uint8_t       _target_cc_out = 0;          // Last written continuity counter in target PID.
        bool          _shift_overflow = false;     // Overflow in target shift buffer, resync at next PUSI.
        size_t        _shift_pusi = NPOS;          // Position of a PUSI in shift buffer (NPOS if there is none).
        ByteBlock     _shift_buffer {};            // Buffer for shifted payload.

        // Process a packet from the target PID, insert PCR when needed, shift payload.
        // Can also be used on the null PID to insert shifted payload.
        void processTargetPacket(TSPacket& pkt);
    };
}
