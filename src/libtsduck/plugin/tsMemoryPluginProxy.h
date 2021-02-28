//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Application communication interface for memory input and output plugins.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMemoryPullHandlerInterface.h"
#include "tsMemoryPushHandlerInterface.h"
#include "tsSingletonManager.h"
#include "tsCondition.h"
#include "tsMutex.h"

namespace ts {
    //!
    //! Application communication interface for memory input and output plugins.
    //! This is a singleton which acts as a proxy between the application and the memory plugins.
    //! @ingroup mpeg
    //!
    //! The application must decide of the communication mode before starting a session.
    //!
    //! Application scenario:
    //!
    //! - Memory input plugin interface - Pull mode:
    //!   - Implement MemoryPullHandlerInterface::pullPackets()
    //!   - Call MemoryPluginProxy::registerInputPullHandler()
    //!   - Call TSProcessor::start()
    //!
    //! - Memory input plugin interface - Push mode:
    //!   - Call MemoryPluginProxy::startPushInput()
    //!   - Call TSProcessor::start()
    //!   - Repeatedly call MemoryPluginProxy::pushInputPackets()
    //!   - Call MemoryPluginProxy::terminatePushInput()
    //!
    //! - Memory output plugin interface - Push mode:
    //!   - Implement MemoryPushHandlerInterface::pushPackets()
    //!   - Call MemoryPluginProxy::registerOutputPushHandler()
    //!   - Call TSProcessor::start()
    //!
    //! - Memory output plugin interface - Pull mode:
    //!   - Call MemoryPluginProxy::startPullOutput()
    //!   - Call TSProcessor::start()
    //!   - Repeatedly call MemoryPluginProxy::pullOutputPackets()
    //!   - Optionally call MemoryPluginProxy::abortPullOutput() if necessary
    //!
    class TSDUCKDLL MemoryPluginProxy
    {
        TS_DECLARE_SINGLETON(MemoryPluginProxy);
    public:
        //!
        //! Memory plugins and applications communicate on a 16-bit "port number", just like TCP or UDP port numbers.
        //!
        //! In practice, the default port number zero can be used, unless there are several instances of TSProcessor
        //! with memory input and output plugins in the same application process. In that case, distinct port numbers
        //! shall be used for distinct memory input plugins (same for memory output port numbers).
        //!
        //! Input port numbers and output port numbers use distincts spaces. The same port number can be used in the
        //! input and output memory plugins without interference.
        //!
        typedef uint16_t PortNumber;

        //!
        //! Singleton destructor.
        //!
        ~MemoryPluginProxy();

        //----------------------------------------------------------------------------
        // Memory input plugin interface - Pull mode
        //----------------------------------------------------------------------------

        //!
        //! Called by the application to register a pull handler on a given port number for memory input plugins.
        //!
        //! In practice, the memory input plugin checks if a pull handler is declared by the application for that
        //! port number. If a handler is registered, it is used to pull packets. If no handler is declared, the
        //! plugin waits on the input queue until the application pushes packets in the queue.
        //!
        //! @param [in] port The port number for the handler.
        //! @param [in] handler The handler to register. Use the null pointer to erase the previous handler.
        //!
        void registerInputPullHandler(PortNumber port, MemoryPullHandlerInterface* handler);

        //!
        //! Called by the memory input plugin to get the pull handler for a given port number.
        //! @param [in] port The port number for the handler.
        //! @return The registered handler for @a port or a null pointer if there is none.
        //!
        MemoryPullHandlerInterface* getInputPullHandler(PortNumber port);

        //----------------------------------------------------------------------------
        // Memory input plugin interface - Push mode
        //----------------------------------------------------------------------------

        //!
        //! Called by the application to start an input session in push mode.
        //! @param [in] port The port number for the input session.
        //!
        void startPushInput(PortNumber port);

        //!
        //! Called by the application to push packets to a memory input plugin.
        //! Return when the input plugin has read the packets.
        //! @param [in] port The port number for the input session.
        //! @param [in] packets Address of input packet buffer.
        //! @param [in] metadata Address of packet metadata buffer. Can be null if there is no metadata.
        //! @param [in] packets_count Number of packets in the buffer.
        //! @return True in case of success, false if there is an output error and the processing chain shall abort.
        //!
        bool pushInputPackets(PortNumber port, const TSPacket* packets, const TSPacketMetadata* metadata, size_t packets_count);

        //!
        //! Called by the application to terminate an input session in push mode.
        //! The input memory plugin will receive an end of input when getting input packets.
        //! @param [in] port The port number for the input session.
        //!
        void terminatePushInput(PortNumber port);

        //!
        //! Called by a memory input plugin to get packets which are pushed by the application.
        //! Return when the application has provided some packets.
        //! @param [in] port The port number for the input session.
        //! @param [out] packets Address of packet buffer to receive.
        //! @param [out] metadata Address of packet metadata buffer.
        //! Can be null if the plugin is not interested in packet metadata.
        //! @param [in] max_packets Maximum number of packets in the buffer.
        //! @return The number of packets which were written in the buffer.
        //! Returning zero means end of input.
        //!
        size_t getPushedInputPackets(PortNumber port, TSPacket* packets, TSPacketMetadata* metadata, size_t max_packets);

        //----------------------------------------------------------------------------
        // Memory output plugin interface - Push mode
        //----------------------------------------------------------------------------

        //!
        //! Called by the application to register a push handler on a given port number for memory output plugins.
        //!
        //! In practice, the memory output plugin checks if a push handler is declared by the application for that
        //! port number. If a handler is registered, it is used to push packets. If no handler is declared, the
        //! plugin writes packet on the output queue from where the application can pull them.
        //!
        //! @param [in] port The port number for the handler.
        //! @param [in] handler The handler to register. Use the null pointer to erase the previous handler.
        //!
        void registerOutputPushHandler(PortNumber port, MemoryPushHandlerInterface* handler);

        //!
        //! Called by the memory output plugin to get the push handler for a given port number.
        //! @param [in] port The port number for the handler.
        //! @return The registered handler for @a port or a null pointer if there is none.
        //!
        MemoryPushHandlerInterface* getOutputPushHandler(PortNumber port);

        //----------------------------------------------------------------------------
        // Memory output plugin interface - Pull mode
        //----------------------------------------------------------------------------

        //!
        //! Called by the application to start an output session in pull mode.
        //! @param [in] port The port number for the output session.
        //!
        void startPullOutput(PortNumber port);

        //!
        //! Called by the application to pull packets from a memory output plugin.
        //! Return when the output plugin has provided some packets.
        //! @param [in] port The port number for the output session.
        //! @param [out] packets Address of packet buffer to receive.
        //! @param [out] metadata Address of packet metadata buffer.
        //! Can be null if the application is not interested in packet metadata.
        //! @param [in] max_packets Maximum number of packets in the buffer.
        //! @return The number of packets which were written in the buffer.
        //! Returning zero means end of output.
        //!
        size_t pullOutputPackets(PortNumber port, TSPacket* packets, TSPacketMetadata* metadata, size_t max_packets);

        //!
        //! Called by the application to abort a memory output session.
        //! The plugin will be informed of the abort when trying to output packets.
        //! @param [in] port The port number for the output session.
        //!
        void abortPullOutput(PortNumber port);

        //!
        //! Called by a memory output plugin to provide packets which are pulled by the application.
        //! Return when the application has read the packets.
        //! @param [in] port The port number for the input session.
        //! @param [in] packets Address of input packet buffer.
        //! @param [in] metadata Address of packet metadata buffer. Can be null if there is no metadata.
        //! @param [in] packets_count Number of packets in the buffer.
        //! @return True in case of success, false if there is an output error and the processing chain shall abort.
        //!
        bool putPulledOutputPackets(PortNumber port, const TSPacket* packets, const TSPacketMetadata* metadata, size_t packets_count);

    private:
        // This internal class implements a "rendezvous" between two threads: a packet provider and a packet consumer.
        // Same concept as rendezvous in the Ada language.
        class RendezVous
        {
            TS_NOCOPY(RendezVous);
        public:
            // Constructor.
            RendezVous();

            // Internal implementation of corresponding public calls in MemoryPluginProxy.
            void start();
            void stop();
            bool putPackets(const TSPacket* packets, const TSPacketMetadata* metadata, size_t packets_count);
            size_t getPackets(TSPacket* packets, TSPacketMetadata* metadata, size_t max_packets);

        private:
            Mutex                   _mutex;          // Protect the access to the condition variable.
            Condition               _put_completed;  // Signaled when a put operation is completed.
            Condition               _get_completed;  // Signaled when a get operation is completed.
            bool                    _started;        // Packet exchange is started.
            const TSPacket*         _put_packets;
            const TSPacketMetadata* _put_metadata;
            size_t                  _put_count;
            TSPacket*               _get_packets;
            TSPacketMetadata*       _get_metadata;
            size_t                  _get_count;

            // Transfer packets during a rendezvous, with mutex held.
            // Return true if actual packets were transfered.
            bool transferPackets();
        };

        // Get input and output rendezvous. Create if necessary, never null.
        RendezVous* getRendezVous(std::map<PortNumber, RendezVous*>&, PortNumber);

        Mutex _mutex;                                                      // Protect access to next fields.
        std::map<PortNumber, RendezVous*> _input_rendezvous;               // input plugin, pull mode
        std::map<PortNumber, RendezVous*> _output_rendezvous;              // output plugin, push mode
        std::map<PortNumber, MemoryPullHandlerInterface*> _pull_handlers;  // input plugin, pull mode
        std::map<PortNumber, MemoryPushHandlerInterface*> _push_handlers;  // output plugin, push mode
    };
}
