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
        bool      _set_discontinuity = false;
        bool      _clear_discontinuity = false;
        bool      _set_transport_error = false;
        bool      _clear_transport_error = false;
        bool      _set_transport_priority = false;
        bool      _clear_transport_priority = false;
        bool      _set_es_priority = false;
        bool      _clear_es_priority = false;
        bool      _resize_payload = false;
        bool      _no_repeat = false;
        size_t    _payload_size = 0;
        bool      _no_payload = false;
        bool      _pes_payload = false;
        ByteBlock _payload_pattern {};
        ByteBlock _payload_and {};
        ByteBlock _payload_or {};
        ByteBlock _payload_xor {};
        size_t    _offset_pattern = 0;
        ByteBlock _private_data {};
        bool      _clear_private_data = false;
        bool      _clear_pcr = false;
        uint64_t  _new_pcr = 0;
        bool      _clear_opcr = false;
        uint64_t  _new_opcr = 0;
        bool      _set_pid = false;
        PID       _new_pid = PID_NULL;
        bool      _set_pusi = false;
        bool      _clear_pusi = false;
        bool      _set_random_access = false;
        bool      _clear_random_access = false;
        bool      _pack_pes_header = false;
        bool      _set_scrambling = false;
        uint8_t   _new_scrambling = 0;
        bool      _set_cc = false;
        uint8_t   _new_cc = 0;
        bool      _set_splice_countdown = false;
        bool      _clear_splice_countdown = false;
        uint8_t   _new_splice_countdown = 0;
        bool      _delete_rs204 = false;
        ByteBlock _rs204 {};

        // Perform --pack-pes-header on a packet.
        void packPESHeader(TSPacket&);

        // Perform payload operations such as --payload-pattern, --payload-and, etc.
        template <typename Op>
        void updatePayload(TSPacket& pkt, size_t payload_base, const ByteBlock& pattern, Op assign);
    };
}
