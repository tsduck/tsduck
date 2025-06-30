//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        TS_PLUGIN_CONSTRUCTORS(CutoffPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool isRealTime() override {return true;}
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        using CommandQueue = MessageQueue<UString>;

        // Plugin private fields.
        volatile bool    _terminate = false;
        size_t           _max_queued = DEFAULT_MAX_QUEUED_COMMANDS;
        IPAddressSet     _allowedRemote {};
        UDPReceiverArgs  _sock_args {};
        UDPReceiver      _sock {*this};
        CommandQueue     _command_queue {DEFAULT_MAX_QUEUED_COMMANDS};
        TSPacketLabelSet _set_labels {};

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
    Thread(ThreadAttributes().setStackSize(SERVER_THREAD_STACK_SIZE))
{
    // UDP receiver common options.
    _sock_args.defineArgs(*this, true, true);

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
    _max_queued = intValue<size_t>(u"max-queue", DEFAULT_MAX_QUEUED_COMMANDS);

    // Get UDP options.
    bool ok = _sock_args.loadArgs(*this, _sock.parameters().receive_timeout);
    _sock.setParameters(_sock_args);

    // Resolve all allowed remote.
    UStringVector remotes;
    getValues(remotes, u"allow");
    _allowedRemote.clear();
    for (const auto& it : remotes) {
        const IPAddress addr(it, *this);
        if (addr.hasAddress()) {
            _allowedRemote.insert(addr);
        }
        else {
            ok = false;
        }
    }

    return ok;
}


//----------------------------------------------------------------------------
// Start method.
//----------------------------------------------------------------------------

bool ts::CutoffPlugin::start()
{
    // Create UDP socket
    if (!_sock.open(*this)) {
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
    _sock.close(*this);

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
    while (_command_queue.dequeue(cmd, cn::milliseconds::zero())) {
        if (cmd != nullptr) {
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
                warning(u"received invalid command \"%s\"", *cmd);
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
    debug(u"server thread started");

    char inbuf[1024];
    size_t insize = 0;
    IPSocketAddress sender;
    IPSocketAddress destination;

    // Get receive errors in a buffer since some errors are normal.
    ReportBuffer<ts::ThreadSafety::None> error(tsp->maxSeverity());

    // Loop on incoming messages.
    while (_sock.receive(inbuf, sizeof(inbuf), insize, sender, destination, tsp, error)) {

        // Filter out unauthorized remote systems.
        if (!_allowedRemote.empty() && !_allowedRemote.contains(sender)) {
            warning(u"rejected remote command from unauthorized host %s", sender);
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
        verbose(u"received command \"%s\", from %s (%d bytes)", *cmd, sender, insize);

        // Enqueue the command immediately. Never wait.
        if (!cmd->empty()) {
            _command_queue.enqueue(cmd, cn::milliseconds::zero());
        }
    }

    // If termination was requested, receive error is not an error.
    if (!_terminate && !error.empty()) {
        info(error.messages());
    }

    debug(u"server thread completed");
}
