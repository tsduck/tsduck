//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//  Transport stream processor shared library:
//  Set labels on TS packets upon reception of UDP messages.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsReportBuffer.h"
#include "tsUDPReceiver.h"
#include "tsMessageQueue.h"
#include "tsThread.h"
#include "tsAlgorithm.h"

#define DEFAULT_MAX_QUEUED_COMMANDS  128
#define SERVER_THREAD_STACK_SIZE     (128 * 1024)


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class CutoffPlugin: public ProcessorPlugin, private Thread
    {
        TS_NOBUILD_NOCOPY(CutoffPlugin);
    public:
        // Implementation of plugin API
        CutoffPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool isRealTime() override {return true;}
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        typedef MessageQueue<UString, Mutex> CommandQueue;

        // Plugin private fields.
        volatile bool    _terminate;      // Force termination flag for thread.
        size_t           _max_queued;     // Max number of queued commands.
        IPv4AddressSet   _allowedRemote;  // Set of allowed remotes.
        UDPReceiver      _sock;           // Incoming socket with associated command line options
        CommandQueue     _command_queue;  // Queue of commands between the UDP server and the plugin thread.
        TSPacketLabelSet _set_labels;     // Labels to set on all packets.

        // Invoked in the context of the server thread.
        virtual void main() override;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"cutoff", ts::CutoffPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::CutoffPlugin::CutoffPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Set labels on TS packets upon reception of UDP messages", u"[options] [address:]port"),
    Thread(ThreadAttributes().setStackSize(SERVER_THREAD_STACK_SIZE)),
    _terminate(false),
    _max_queued(DEFAULT_MAX_QUEUED_COMMANDS),
    _allowedRemote(),
    _sock(*tsp_),
    _command_queue(DEFAULT_MAX_QUEUED_COMMANDS),
    _set_labels()
{
    // UDP receiver common options.
    _sock.defineArgs(*this, true, true, false);

    option(u"allow", 'a', STRING);
    help(u"allow", u"address",
        u"Specify an IP address or host name which is allowed to send remote commands. "
        u"Several --allow options are allowed. By default, all remote commands are accepted.");

    option(u"max-queue", 0, POSITIVE);
    help(u"max-queue",
         u"Specify the maximum number of queued UDP commands before their execution "
         u"into the stream. The default is " TS_STRINGIFY(DEFAULT_MAX_QUEUED_COMMANDS) u".");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::CutoffPlugin::getOptions()
{
    bool ok = true;
    _max_queued = intValue<size_t>(u"max-queue", DEFAULT_MAX_QUEUED_COMMANDS);

    // Resolve all allowed remote.
    UStringVector remotes;
    getValues(remotes, u"allow");
    _allowedRemote.clear();
    for (const auto& it : remotes) {
        const IPv4Address addr(it, *tsp);
        if (addr.hasAddress()) {
            _allowedRemote.insert(addr);
        }
        else {
            ok = false;
        }
    }

    // Get UDP options.
    return _sock.loadArgs(duck, *this) && ok;
}


//----------------------------------------------------------------------------
// Start method.
//----------------------------------------------------------------------------

bool ts::CutoffPlugin::start()
{
    // Create UDP socket
    if (!_sock.open(*tsp)) {
        return false;
    }

    // Reset buffers.
    _command_queue.clear();
    _command_queue.setMaxMessages(_max_queued);
    _set_labels.reset();

    // Start the internal thread which listens to incoming UDP packet.
    _terminate = false;
    Thread::start();
    return true;
}


//----------------------------------------------------------------------------
// Stop method.
//----------------------------------------------------------------------------

bool ts::CutoffPlugin::stop()
{
    // Close the UDP socket.
    // This will force the server thread to terminate on receive error.
    // In case the server does not properly notify the error, set a flag.
    _terminate = true;
    _sock.close(*tsp);

    // Wait for actual thread termination
    Thread::waitForTermination();
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::CutoffPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Process all enqueued commands from the UDP listener thread.
    CommandQueue::MessagePtr cmd;
    while (_command_queue.dequeue(cmd, 0)) {
        if (!cmd.isNull()) {
            // Split the command from spaces.
            UStringVector argv;
            cmd->split(argv, SPACE, true, true);
            size_t iparam = 0;
            const size_t argc = argv.size();
            const bool is_int = argc >= 2 && argv[1].toInteger(iparam);

            // Execute the command.
            if (argc > 0 && argv[0] == u"exit") {
                // Terminate tsp.
                return TSP_END;
            }
            else if (is_int && argv[0] == u"pulse-label" && iparam <= TSPacketLabelSet::MAX) {
                // Set label on one single packet.
                pkt_data.setLabel(iparam);
            }
            else if (is_int && argv[0] == u"start-label" && iparam <= TSPacketLabelSet::MAX) {
                // Set this label on all packets.
                _set_labels.set(iparam);
            }
            else if (is_int && argv[0] == u"stop-label" && iparam <= TSPacketLabelSet::MAX) {
                // Stop setting this label on all packets.
                _set_labels.reset(iparam);
            }
            else {
                tsp->warning(u"received invalid command \"%s\"", {*cmd});
            }
        }
    }

    // Set labels on all packets.
    pkt_data.setLabels(_set_labels);
    return TSP_OK;
}


//----------------------------------------------------------------------------
// Invoked in the context of the server thread.
//----------------------------------------------------------------------------

void ts::CutoffPlugin::main()
{
    tsp->debug(u"server thread started");

    char inbuf[1024];
    size_t insize = 0;
    IPv4SocketAddress sender;
    IPv4SocketAddress destination;

    // Get receive errors in a buffer since some errors are normal.
    ReportBuffer<NullMutex> error(tsp->maxSeverity());

    // Loop on incoming messages.
    while (_sock.receive(inbuf, sizeof(inbuf), insize, sender, destination, tsp, error)) {

        // Filter out unauthorized remote systems.
        if (!_allowedRemote.empty() && !Contains(_allowedRemote, sender)) {
            tsp->warning(u"rejected remote command from unauthorized host %s", {sender});
            continue;
        }

        // We expect ASCII commands. Locate first non-ASCII character in message.
        size_t len = 0;
        while (len < insize && inbuf[len] >= 0x20 && inbuf[len] <= 0x7E) {
            len++;
        }

        // Extract trimmed lowercase ASCII command.
        CommandQueue::MessagePtr cmd(new UString(UString::FromUTF8(inbuf, len)));
        cmd->toLower();
        cmd->trim();
        tsp->verbose(u"received command \"%s\", from %s (%d bytes)", {*cmd, sender, insize});

        // Enqueue the command immediately. Never wait.
        if (!cmd->empty()) {
            _command_queue.enqueue(cmd, 0);
        }
    }

    // If termination was requested, receive error is not an error.
    if (!_terminate && !error.emptyMessages()) {
        tsp->info(error.getMessages());
    }

    tsp->debug(u"server thread completed");
}
