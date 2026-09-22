//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to inject MPE (Multi-Protocol Encapsulation) datagrams in a TS.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsUDPReceiver.h"
#include "tsUDPReceiverArgsList.h"
#include "tsMACAddress.h"
#include "tsPacketizer.h"
#include "tsMessageQueue.h"
#include "tsThread.h"

namespace ts {
    //!
    //! Plugin to inject MPE (Multi-Protocol Encapsulation) datagrams in a transport stream.
    //! @see ETSI EN 301 192.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL MPEInjectPlugin: public ProcessorPlugin, private SectionProviderInterface
    {
        TS_PLUGIN_CONSTRUCTORS(MPEInjectPlugin);
    public:
        // Implementation of plugin API.
        virtual bool start() override;
        virtual bool getOptions() override;
        virtual bool stop() override;
        virtual bool isRealTime() override {return true;}
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Each UDP receiver is executed in a thread. There is a vector of receiver threads.
        class ReceiverThread;
        using ReceiverPtr = std::shared_ptr<ReceiverThread>;
        using ReceiverVector = std::vector<ReceiverPtr>;

        // Each receiver thread builds DSM-CC sections from the received UDP datagrams.
        // Sections from all receivers are multiplexed into one single thread-safe queue.
        using SectionQueue = MessageQueue<Section>;

        static constexpr size_t DEFAULT_MAX_QUEUED_SECTION = 32;
        static constexpr size_t SERVER_THREAD_STACK_SIZE = 128 * 1024;
        static constexpr size_t OVERFLOW_MSG_GROUP_COUNT = 1000;

        // Command line options.
        PID        _mpe_pid = PID_NULL;     // PID into insert the MPE datagrams.
        bool       _replace = false;        // Replace incoming PID if it exists.
        bool       _pack_sections = false;  // Packet DSM-CC section, without stuffing in TS packets.
        size_t     _max_queued = DEFAULT_MAX_QUEUED_SECTION; // Max number of queued sections.
        MACAddress _default_mac {};         // Default MAC address in MPE section for unicast packets.
        UDPReceiverArgsList _recv_args {};  // Receiver options.

        // Working data.
        volatile bool  _terminate = false;  // Force termination flag for thread.
        SectionQueue   _section_queue {DEFAULT_MAX_QUEUED_SECTION};  // Queue of datagrams between the UDP server and the MPE inserter.
        Packetizer     _packetizer {duck, PID_NULL, this};           // Packetizer for MPE sections.
        ReceiverVector _receivers {};       // UDP receiver threads.

        // Implementation of SectionProviderInterface.
        virtual void provideSection(SectionCounter counter, SectionPtr& section) override;
        virtual bool doStuffing() override;

        // Each UDP receiver is executed in a thread of this class.
        class ReceiverThread: public Thread
        {
            TS_NOBUILD_NOCOPY(ReceiverThread);
        public:
            // Constructor.
            ReceiverThread(MPEInjectPlugin* plugin, const UDPReceiverArgs& opt, size_t index, size_t receiver_count);

            // Open/close UDP socket.
            bool openSocket() { return _sock.open(); }
            bool closeSocket() { return _sock.close(); }

        protected:
            // Invoked in the context of the server thread.
            virtual void main() override;

        private:
            MPEInjectPlugin* const _plugin;   // Parent plugin.
            IPSocketAddress _new_source {};   // Masquerade source socket in MPE section.
            IPSocketAddress _new_dest {};     // Masquerade destination socket in MPE section.
            UDPReceiver     _sock {_plugin};  // Incoming socket with associated command line options.
            size_t          _index;           // Receiver index.
        };
    };
}
