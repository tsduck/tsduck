//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to regulate (slow down) the packet flow according to a bitrate.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsBitRateRegulator.h"
#include "tsPCRRegulator.h"

namespace ts {
    //!
    //! Plugin to regulate (slow down) the packet flow according to a bitrate.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL RegulatePlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(RegulatePlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool isRealTime() override {return true;}
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Default value for --packet-burst.
        static constexpr PacketCounter DEFAULT_PACKET_BURST = 16;

        // Command line options:
        bool             _pcr_synchronous = false;
        BitRate          _bitrate = 0;
        PacketCounter    _burst = 0;
        cn::milliseconds _wait_min {};
        PID              _pid_pcr = PID_NULL;

        // Working data:
        BitRateRegulator _bitrate_regulator {this, Severity::Verbose};
        PCRRegulator     _pcr_regulator {this, Severity::Verbose};
    };
}
