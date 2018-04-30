//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//
//  Inject SCTE 35 splice commands in a transport stream.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsSpliceInformationTable.h"
#include "tsUDPReceiver.h"
#include "tsPollFiles.h"
#include "tsOneShotPacketizer.h"
#include "tsMessageQueue.h"
#include "tsThread.h"
#include "tsNullReport.h"
TSDUCK_SOURCE;

#define DEFAULT_SECTION_QUEUE_SIZE 100   // Maximum number of sections in queue
#define SERVER_THREAD_STACK_SIZE  (128 * 1024)


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SpliceInjectPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        SpliceInjectPlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        // File listener thread.
        class FileListener : public Thread, private PollFilesListener
        {
        public:
            // Constructor.
            FileListener(SpliceInjectPlugin* plugin);

            // Terminate the thread.
            void stop();

        private:
            TS_UNUSED //@@@@
            SpliceInjectPlugin* const _plugin;
            TSP* const _tsp;
            bool _terminate;

            // Invoked in the context of the server thread.
            virtual void main() override;

            // Implementation of PollFilesListener.
            virtual bool handlePolledFiles(const PolledFileList& files) override;
            virtual bool updatePollFiles(UString& wildcard, MilliSecond& poll_interval, MilliSecond& min_stable_delay) override;

            // Inaccessible operations.
            FileListener() = delete;
            FileListener(const FileListener&) = delete;
            FileListener& operator=(const FileListener&) = delete;
        };

        // UDP listener thread.
        class UDPListener : public Thread
        {
        public:
            // Constructor.
            UDPListener(SpliceInjectPlugin* plugin);

            // Open the UDP socket.
            bool open();

            // Terminate the thread.
            void stop();

        private:
            SpliceInjectPlugin* const _plugin;
            TSP* const  _tsp;
            UDPReceiver _client;

            // Invoked in the context of the server thread.
            virtual void main() override;

            // Inaccessible operations.
            UDPListener() = delete;
            UDPListener(const UDPListener&) = delete;
            UDPListener& operator=(const UDPListener&) = delete;
        };

        // Plugin private data
        SocketAddress _server_address;       // TCP/UDP port and optional local address.
        bool          _reuse_port;           // Reuse port option.
        size_t        _sock_buf_size;        // Socket receive buffer size.
        FileListener  _file_listener;        // TCP listener thread.
        UDPListener   _udp_listener;         // UDP listener thread.

        // Inaccessible operations
        SpliceInjectPlugin() = delete;
        SpliceInjectPlugin(const SpliceInjectPlugin&) = delete;
        SpliceInjectPlugin& operator=(const SpliceInjectPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(spliceinject, ts::SpliceInjectPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SpliceInjectPlugin::SpliceInjectPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Inject SCTE 35 splice commands in a transport stream.", u"[options]"),
    _server_address(),
    _reuse_port(false),
    _sock_buf_size(0),
    _file_listener(this),
    _udp_listener(this)
{
    setHelp( u"Options:\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::SpliceInjectPlugin::start()
{
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::SpliceInjectPlugin::stop()
{
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::SpliceInjectPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    return TSP_OK;
}


//----------------------------------------------------------------------------
// File listener thread.
//----------------------------------------------------------------------------

ts::SpliceInjectPlugin::FileListener::FileListener(SpliceInjectPlugin* plugin) :
    Thread(ThreadAttributes().setStackSize(SERVER_THREAD_STACK_SIZE)),
    _plugin(plugin),
    _tsp(plugin->tsp),
    _terminate(false)
{
}

// Terminate the thread.
void ts::SpliceInjectPlugin::FileListener::stop()
{
    // Will be used at next poll.
    _terminate = true;

    // Wait for actual thread termination
    Thread::waitForTermination();
}


// Invoked in the context of the server thread.
void ts::SpliceInjectPlugin::FileListener::main()
{
    _tsp->debug(u"file server thread started");

    // @@@@@@

    _tsp->debug(u"file server thread completed");
}

// Invoked before polling.
bool ts::SpliceInjectPlugin::FileListener::updatePollFiles(UString& wildcard, MilliSecond& poll_interval, MilliSecond& min_stable_delay)
{
    return !_terminate;
}

// Invoked with modified files.
bool ts::SpliceInjectPlugin::FileListener::handlePolledFiles(const PolledFileList& files)
{
    //@@@@@@
    return !_terminate;
}


//----------------------------------------------------------------------------
// UDP listener thread.
//----------------------------------------------------------------------------

ts::SpliceInjectPlugin::UDPListener::UDPListener(SpliceInjectPlugin* plugin) :
    Thread(ThreadAttributes().setStackSize(SERVER_THREAD_STACK_SIZE)),
    _plugin(plugin),
    _tsp(plugin->tsp),
    _client(*plugin->tsp)
{
}

// Open the UDP socket.
bool ts::SpliceInjectPlugin::UDPListener::open()
{
    _client.setParameters(_plugin->_server_address, _plugin->_reuse_port, _plugin->_sock_buf_size);
    return _client.open(*_tsp);
}

// Terminate the thread.
void ts::SpliceInjectPlugin::UDPListener::stop()
{
    // Close the UDP receiver.
    // This will force the server thread to terminate.
    _client.close(NULLREP);

    // Wait for actual thread termination
    Thread::waitForTermination();
}

// Invoked in the context of the server thread.
void ts::SpliceInjectPlugin::UDPListener::main()
{
    _tsp->debug(u"UDP server thread started");

    uint8_t inbuf[65536];
    size_t insize = 0;
    SocketAddress sender;
    SocketAddress destination;

    // Loop on incoming messages.
    while (_client.receive(inbuf, sizeof(inbuf), insize, sender, destination, _tsp, *_tsp)) {
        // @@@@@@
    }

    _tsp->debug(u"UDP server thread completed");
}
