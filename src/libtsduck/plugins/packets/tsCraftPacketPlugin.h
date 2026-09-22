//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Craft packet processor plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"

namespace ts {
    //!
    //! Craft packet processor plugin for tsp.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL CraftPacketPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(CraftPacketPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        bool      _setDiscontinuity = false;
        bool      _clearDiscontinuity = false;
        bool      _setTransportError = false;
        bool      _clearTransportError = false;
        bool      _setTransportPriority = false;
        bool      _clearTransportPriority = false;
        bool      _setESPriority = false;
        bool      _clearESPriority = false;
        bool      _resizePayload = false;
        bool      _noRepeat = false;
        size_t    _payloadSize = 0;
        bool      _noPayload = false;
        bool      _pesPayload = false;
        ByteBlock _payloadPattern {};
        ByteBlock _payloadAnd {};
        ByteBlock _payloadOr {};
        ByteBlock _payloadXor {};
        size_t    _offsetPattern = 0;
        ByteBlock _privateData {};
        bool      _clearPrivateData = false;
        bool      _clearPCR = false;
        uint64_t  _newPCR = 0;
        bool      _clearOPCR = false;
        uint64_t  _newOPCR = 0;
        bool      _setPID = false;
        PID       _newPID = PID_NULL;
        bool      _setPUSI = false;
        bool      _clearPUSI = false;
        bool      _setRandomAccess = false;
        bool      _clearRandomAccess = false;
        bool      _packPESHeader = false;
        bool      _setScrambling = false;
        uint8_t   _newScrambling = 0;
        bool      _setCC = false;
        uint8_t   _newCC = 0;
        bool      _setSpliceCountdown = false;
        bool      _clearSpliceCountdown = false;
        uint8_t   _newSpliceCountdown = 0;
        bool      _deleteRS204 = false;
        ByteBlock _rs204 {};

        // Perform --pack-pes-header on a packet.
        void packPESHeader(TSPacket&);

        // Perform payload operations such as --payload-pattern, --payload-and, etc.
        template <typename Op>
        void updatePayload(TSPacket& pkt, size_t payloadBase, const ByteBlock& pattern, Op assign);
    };
}
