//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to boost the bitrate of a PID, stealing packets from stuffing.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"

namespace ts {
    //!
    //! Plugin to boost the bitrate of a PID, stealing packets from stuffing.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL BoostPIDPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(BoostPIDPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        uint16_t _pid = PID_NULL;  // Target PID
        int      _opt_addpkt = 0;  // addpkt in addpkt/inpkt parameter
        int      _opt_inpkt = 0;   // inpkt in addpkt/inpkt parameter

        // Working data:
        uint8_t  _last_cc = 0;     // Last continuity counter in PID
        int      _in_count = 0;    // Input packet countdown for next insertion
        int      _add_count = 0;   // Current number of packets to add
    };
}
