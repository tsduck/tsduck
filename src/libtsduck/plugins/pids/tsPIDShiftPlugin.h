//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to shift one or more PID's forward in the transport stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsTimeShiftBuffer.h"

namespace ts {
    //!
    //! Plugin to shift one or more PID's forward in the transport stream.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL PIDShiftPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PIDShiftPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        bool             _ignore_errors = false;  // Ignore evaluation errors.
        size_t           _shift_packets = 0;      // Shift buffer size in packets.
        cn::milliseconds _shift_ms {};            // Shift buffer size in milliseconds.
        cn::milliseconds _eval_ms {};             // Initial evaluation phase duration in milliseconds.
        PIDSet           _pids {};                // List of PID's to shift forward.

        // Working data:
        bool             _pass_all = false;       // Pass all packets after an error.
        PacketCounter    _init_packets = 0;       // Count packets in PID's to shift during initial evaluation phase.
        TimeShiftBuffer  _buffer {this};          // The timeshift buffer logic.

        static constexpr cn::milliseconds DEF_EVAL_MS = cn::milliseconds(1000);  // Default initial evaluation duration in milliseconds.
        static constexpr PacketCounter MAX_EVAL_PACKETS = 30000;                 // Max number of packets after which the bitrate must be known.
    };
}
