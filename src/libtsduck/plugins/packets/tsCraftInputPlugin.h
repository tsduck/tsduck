//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Craft input plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsInputPlugin.h"

namespace ts {
    //!
    //! Craft input plugin for tsp.
    //! Build specifically crafted input packets.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL CraftInputPlugin: public InputPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(CraftInputPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;
        virtual bool abortInput() override;
        virtual bool setReceiveTimeout(cn::milliseconds timeout) override;

    private:
        // Command line options:
        uint8_t       _initCC = 0;          // continuity_counter
        bool          _constantCC = false;  // Do not increment continuity counter
        PacketCounter _maxCount = 0;        // Number of packets to generate

        // Working data:
        PacketCounter    _limit = 0;            // Current max number of packets
        TSPacket         _packet {NullPacket};  // Template of packet to generate
        TSPacketMetadata _mdata {};             // Metadata of packet to generate
    };
}
