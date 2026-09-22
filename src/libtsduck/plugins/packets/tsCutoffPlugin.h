//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to set labels on TS packets upon reception of UDP messages.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsUDPReceiver.h"
#include "tsMessageQueue.h"
#include "tsThread.h"

namespace ts {
    //!
    //! Plugin to set labels on TS packets upon reception of UDP messages.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL CutoffPlugin: public ProcessorPlugin, private Thread
    {
        TS_PLUGIN_CONSTRUCTORS(CutoffPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool isRealTime() override {return true;}
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        using CommandQueue = MessageQueue<UString>;

        static constexpr size_t DEFAULT_MAX_QUEUED_COMMANDS = 128;
        static constexpr size_t SERVER_THREAD_STACK_SIZE = 128 * 1024;

        // Plugin private fields.
        volatile bool    _terminate = false;
        size_t           _max_queued = DEFAULT_MAX_QUEUED_COMMANDS;
        IPAddressSet     _allowedRemote {};
        UDPReceiverArgs  _sock_args {};
        UDPReceiver      _sock {this};
        CommandQueue     _command_queue {DEFAULT_MAX_QUEUED_COMMANDS};
        TSPacketLabelSet _set_labels {};

        // Invoked in the context of the server thread.
        virtual void main() override;
    };
}
