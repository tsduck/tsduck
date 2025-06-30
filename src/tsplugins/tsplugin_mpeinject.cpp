//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Inject MPE (Multi-Protocol Encapsulation) datagrams in a transport stream.
//  See ETSI EN 301 192.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsUDPReceiver.h"
#include "tsUDPReceiverArgsList.h"
#include "tsMPEPacket.h"
#include "tsPacketizer.h"
#include "tsMessageQueue.h"
#include "tsThread.h"

#define MAX_IP_SIZE                 65536
#define DEFAULT_MAX_QUEUED_SECTION  32
#define SERVER_THREAD_STACK_SIZE    (128 * 1024)
#define OVERFLOW_MSG_GROUP_COUNT    1000


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class MPEInjectPlugin: public ProcessorPlugin, private SectionProviderInterface
    {
        TS_PLUGIN_CONSTRUCTORS(MPEInjectPlugin);
    public:
        // Implementation of plugin API.
        virtual bool start() override;
        virtual bool getOptions() override;
        virtual bool stop() override;
        virtual bool isRealTime() override {return true;}
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Each UDP receiver is executed in a thread. There is a vector of receiver threads.
        class ReceiverThread;
        using ReceiverPtr = std::shared_ptr<ReceiverThread>;
        using ReceiverVector = std::vector<ReceiverPtr>;

        // Each receiver thread builds DSM-CC sections from the received UDP datagrams.
        // Sections from all receivers are multiplexed into one single thread-safe queue.
        using SectionQueue = MessageQueue<Section>;

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
            bool openSocket() { return _sock.open(*_plugin); }
            bool closeSocket() { return _sock.close(*_plugin); }

        protected:
            // Invoked in the context of the server thread.
            virtual void main() override;

        private:
            MPEInjectPlugin* const _plugin;  // Parent plugin.
            IPSocketAddress _new_source {};  // Masquerade source socket in MPE section.
            IPSocketAddress _new_dest {};    // Masquerade destination socket in MPE section.
            UDPReceiver     _sock;           // Incoming socket with associated command line options.
            size_t          _index;          // Receiver index.
        };
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"mpeinject", ts::MPEInjectPlugin);


//----------------------------------------------------------------------------
// Constructor for the plugin.
//----------------------------------------------------------------------------

ts::MPEInjectPlugin::MPEInjectPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Inject an incoming UDP stream into MPE (Multi-Protocol Encapsulation)", u"[options] [address:]port ...")
{
    _recv_args.defineArgs(*this, true, true, true);

    option(u"mac-address", 0, STRING);
    help(u"mac-address", u"nn:nn:nn:nn:nn:nn",
         u"Specify the default destination MAC address to set in MPE sections for "
         u"unicast IP packets. The default is 00:00:00:00:00:00. For multicast IP "
         u"packets, the MAC address is automatically computed.");

    option(u"max-queue", 0, POSITIVE);
    help(u"max-queue",
         u"Specify the maximum number of queued UDP datagrams before their insertion "
         u"into the MPE stream. The default is " TS_STRINGIFY(DEFAULT_MAX_QUEUED_SECTION) u".");

    option(u"new-destination", 0, IPSOCKADDR_OP, 0, UNLIMITED_COUNT);
    help(u"new-destination",
         u"Change the destination IP address and UDP port in MPE sections. "
         u"If the port is not specified, the original destination port from the UDP datagram is used. "
         u"By default, the destination address is not modified.\n"
         u"If several [address:]port parameters are specified, several --new-destination options can "
         u"be specified, one for each receiver, in the same order. "
         u"It there are less --new-destination options than receivers, the last --new-destination "
         u"applies for all remaining receivers.");

    option(u"new-source", 0, IPSOCKADDR_OP, 0, UNLIMITED_COUNT);
    help(u"new-source",
         u"Change the source IP address and UDP port in MPE sections. If the port is "
         u"not specified, the original source port from the UDP datagram is used. By "
         u"default, the source address is not modified.\n"
         u"If several [address:]port parameters are specified, several --new-source options can "
         u"be specified, one for each receiver, in the same order. "
         u"It there are less --new-source options than receivers, the last --new-source "
         u"applies for all remaining receivers.");

    option(u"pack-sections");
    help(u"pack-sections",
         u"Specify to pack DSM-CC sections containing MPE datagrams. "
         u"With this option, each DSM-CC section starts in the same TS packet as the previous section. "
         u"By default, the last TS packet of a DSM-CC section is stuffed and the next section starts in the next TS packet of the PID.");

    option(u"pid", 'p', PIDVAL, 1, 1);
    help(u"pid",
         u"Specify the PID into which the MPE datagrams shall be inserted. This is a "
         u"mandatory parameter.");

    option(u"replace");
    help(u"replace",
         u"Replace the target PID if it exists. By default, the plugin only replaces "
         u"null packets and tsp stops with an error if incoming packets are found "
         u"with the target PID.");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::MPEInjectPlugin::getOptions()
{
    // Get common options, not depending on a receiver.
    _mpe_pid = intValue<PID>(u"pid");
    _max_queued = intValue<size_t>(u"max-queue", DEFAULT_MAX_QUEUED_SECTION);
    _replace = present(u"replace");
    _pack_sections = present(u"pack-sections");
    const UString mac_address(value(u"mac-address"));
    if (!mac_address.empty() && !_default_mac.resolve(mac_address, *this)) {
        return false;
    }
    if (!_recv_args.loadArgs(*this)) {
        return false;
    }

    // Clearing the vector of receivers automatically deallocated previous receivers (if any).
    _receivers.clear();

    // Create all receivers and set options.
    for (size_t i = 0; i < _recv_args.size(); ++i) {
        _receivers.push_back(std::make_shared<ReceiverThread>(this, _recv_args[i], i, _recv_args.size()));
    }

    return !gotErrors();
}


//----------------------------------------------------------------------------
// Constructor for a receiver thread.
//----------------------------------------------------------------------------

ts::MPEInjectPlugin::ReceiverThread::ReceiverThread(MPEInjectPlugin* plugin, const UDPReceiverArgs& opt, size_t index, size_t receiver_count) :
    Thread(ThreadAttributes().setStackSize(SERVER_THREAD_STACK_SIZE)),
    _plugin(plugin),
    _sock(*_plugin),
    _index(index)
{
    // Set UDP socket options.
    _sock.setParameters(opt);

    // Get optional new source and destination.
    const size_t dst_count = _plugin->count(u"new-destination");
    const size_t src_count = _plugin->count(u"new-source");

    if (dst_count > receiver_count) {
        _plugin->error(u"too many --new-destination options");
    }
    if (src_count > receiver_count) {
        _plugin->error(u"too many --new-source options");
    }
    if (dst_count > 0) {
        _plugin->getSocketValue(_new_dest, u"new-destination", IPSocketAddress(), std::min(_index, dst_count - 1));
    }
    if (src_count > 0) {
        _plugin->getSocketValue(_new_source, u"new-source", IPSocketAddress(), std::min(_index, src_count - 1));
    }
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::MPEInjectPlugin::start()
{
    // Create all UDP sockets.
    for (size_t i = 0; i < _receivers.size(); ++i) {
        if (!_receivers[i]->openSocket()) {
            // Failed to open one socket, clase those which were alread opened.
            for (size_t old = 0; old < i; ++old) {
                _receivers[old]->closeSocket();
            }
            return false;
        }
    }

    // Reset section queue.
    _section_queue.clear();
    _section_queue.setMaxMessages(_max_queued);

    // Reset packetizer.
    _packetizer.reset();
    _packetizer.setPID(_mpe_pid);

    // Start all internal threads which listens to incoming UDP packet.
    _terminate = false;
    for (auto& rec : _receivers) {
        rec->start();
    }

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::MPEInjectPlugin::stop()
{
    // Close all UDP sockets. This will force the server threads to terminate on receive error.
    // In case the server does not properly notify the error, set a volatile flag.
    _terminate = true;
    for (auto& rec : _receivers) {
        rec->closeSocket();
    }

    // Wait for actual thread terminations.
    for (auto& rec : _receivers) {
        rec->waitForTermination();
    }
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::MPEInjectPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Abort if data PID is already present in TS and --replace is not specified.
    const PID pid = pkt.getPID();
    if (!_replace && pid == _mpe_pid) {
        error(u"MPE PID conflict, specified %n, now found as input PID, try another one", pid);
        return TSP_END;
    }

    // MPE injection occurs by replacing original PID or null packets.
    if ((_replace && pid == _mpe_pid) || (!_replace && pid == PID_NULL)) {
        // Get next packet from the packetizer (can be a null packet if there is no section available).
        _packetizer.getNextPacket(pkt);
    }

    return TSP_OK;
}


//----------------------------------------------------------------------------
// Implementation of SectionProviderInterface, invoked by the packetizer.
//----------------------------------------------------------------------------

bool ts::MPEInjectPlugin::doStuffing()
{
    // Indicate if we need to stuff TS packets between packetized sections.
    return !_pack_sections;
}

void ts::MPEInjectPlugin::provideSection(SectionCounter counter, SectionPtr& section)
{
    // The packetizer needs a new section to packetize. Try to get next section.
    // Do not wait for a section, just do nothing if there is none available.
    if (!_section_queue.dequeue(section, cn::milliseconds::zero()) || section == nullptr || !section->isValid()) {
        // No section available. Clear returned pointer, just in case.
        section.reset();
    }
}


//----------------------------------------------------------------------------
// Invoked in the context of the server thread.
//----------------------------------------------------------------------------

void ts::MPEInjectPlugin::ReceiverThread::main()
{
    _plugin->debug(u"UDP reception thread %d started", _index);

    // Try to cumlate "UDP overflow" messages.
    size_t overflow_count = 0;

    size_t insize;
    IPSocketAddress sender;
    IPSocketAddress destination;
    ByteBlock buffer(MAX_IP_SIZE);

    // Loop on message reception until a receive error (probably an end of execution).
    while (!_plugin->_terminate && _sock.receive(buffer.data(), buffer.size(), insize, sender, destination, _plugin->tsp, *_plugin)) {

        // Rebuild source and destination addresses if required.
        if (_new_source.hasAddress()) {
            sender.setAddress(_new_source);
        }
        if (_new_source.hasPort()) {
            sender.setPort(_new_source.port());
        }
        if (_new_dest.hasAddress()) {
            destination.setAddress(_new_dest);
        }
        if (_new_dest.hasPort()) {
            destination.setPort(_new_dest.port());
        }

        // Compute destination MAC address for MPE section.
        MACAddress mac(_plugin->_default_mac);
        if (destination.isMulticast()) {
            mac.toMulticast(destination);
        }

        // Create an MPE packet containing this datagram.
        MPEPacket mpe;
        mpe.setSourcePID(_plugin->_mpe_pid);
        mpe.setSourceSocket(sender);
        mpe.setDestinationSocket(destination);
        mpe.setDestinationMACAddress(mac);
        mpe.setUDPMessage(buffer.data(), insize);

        // Create an MPE section for the datagram.
        SectionQueue::MessagePtr section(new Section());
        mpe.createSection(*section);

        // Enqueue the section immediately. Never wait.
        if (!section->isValid()) {
            _plugin->error(u"error creating MPE section from UDP datagram, source: %s, destination: %s, size: %d bytes", sender, destination, insize);
        }
        else {
            const bool queued = _plugin->_section_queue.enqueue(section, cn::milliseconds::zero());
            if (!queued) {
                overflow_count++;
            }
            if ((queued && overflow_count > 0) || overflow_count >= OVERFLOW_MSG_GROUP_COUNT) {
                _plugin->warning(u"incoming UDP overflow, dropped %d datagrams", overflow_count);
                overflow_count = 0;
            }
        }
    }

    _plugin->debug(u"UDP reception thread %d completed", _index);
}
