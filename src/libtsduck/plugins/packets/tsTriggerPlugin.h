//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to trigger actions on selected labeled TS packets.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsForkPipe.h"
#include "tsByteBlock.h"
#include "tsUDPSocket.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Plugin to trigger actions on selected labeled TS packets.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL TriggerPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(TriggerPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        PacketCounter      _min_inter_packet = 0; // Minimum interval in packets between two actions.
        cn::milliseconds   _min_inter_time {};    // Minimum interval in milliseconds between two actions.
        UString            _execute {};           // Command to execute on trigger.
        fs::path           _copy_source {};       // Copy that file ...
        fs::path           _copy_dest {};         // ... into this destination.
        IPSocketAddress    _udp_destination {};   // UDP/IP destination address:port.
        IPAddress          _udp_local {};         // Name of outgoing local address (empty if unspecified).
        ByteBlock          _udp_message {};       // What to send as UDP message.
        int                _udp_ttl = 0;          // Time-to-live socket option.
        bool               _on_start = false;     // Trigger action on start.
        bool               _on_stop = false;      // Trigger action on stop.
        bool               _all_packets = false;  // Trigger on all packets in the stream.
        bool               _all_labels = false;   // Need all labels to be set.
        bool               _once = false;         // Trigger the actions only once per label.
        TSPacketLabelSet   _labels {};            // Trigger on packets with these labels, from options.
        ForkPipe::WaitMode _wait_mode = ForkPipe::ASYNCHRONOUS;  // How to run executed commands.

        // Working data:
        PacketCounter    _last_packet = INVALID_PACKET_COUNTER; // Last action packet.
        Time             _last_time {};           // UTC time of last action.
        UDPSocket        _sock {this};            // Output socket.
        TSPacketLabelSet _current_labels {};      // Trigger on packets with these labels, during processing.

        // Trigger the actions (exec, UDP).
        void trigger();
    };
}
