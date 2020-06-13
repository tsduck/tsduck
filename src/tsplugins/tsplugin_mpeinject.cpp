//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//  Inject MPE (Multi-Protocol Encapsulation) datagrams in a transport stream.
//  See ETSI EN 301 192.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsUDPReceiver.h"
#include "tsMPEPacket.h"
#include "tsOneShotPacketizer.h"
#include "tsContinuityAnalyzer.h"
#include "tsMessageQueue.h"
#include "tsThread.h"
TSDUCK_SOURCE;

#define MAX_IP_SIZE                 65536
#define DEFAULT_MAX_QUEUED_SECTION  32
#define SERVER_THREAD_STACK_SIZE    (128 * 1024)
#define OVERFLOW_MSG_GROUP_COUNT    1000


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class MPEInjectPlugin: public ProcessorPlugin, private Thread
    {
        TS_NOBUILD_NOCOPY(MPEInjectPlugin);
    public:
        // Implementation of plugin API
        MPEInjectPlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool isRealTime() override {return true;}
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        typedef MessageQueue<Section, Mutex> SectionQueue;

        // Plugin private fields.
        volatile bool  _terminate;      // Force termination flag for thread.
        PID            _mpe_pid;        // PID into insert the MPE datagrams.
        bool           _replace;        // Replace incoming PID if it exists.
        size_t         _max_queued;     // Max number of queued sections.
        MACAddress     _default_mac;    // Default MAC address in MPE section for unicast packets.
        SocketAddress  _new_source;     // Masquerade source socket in MPE section.
        SocketAddress  _new_dest;       // Masquerade destination socket in MPE section.
        UDPReceiver    _sock;           // Incoming socket with associated command line options
        SectionQueue   _section_queue;  // Queue of datagrams between the UDP server and the MPE inserter.
        TSPacketVector _mpe_packets;    // TS packets to insert, contain a packetized MPE section.
        size_t         _packet_index;   // Next index in _mpe_packets.
        ContinuityAnalyzer _cc_fixer;   // To fix continuity counters in MPE PID.

        // Invoked in the context of the server thread.
        virtual void main() override;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"mpeinject", ts::MPEInjectPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::MPEInjectPlugin::MPEInjectPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Inject an incoming UDP stream into MPE (Multi-Protocol Encapsulation)", u"[options] [address:]port"),
    Thread(ThreadAttributes().setStackSize(SERVER_THREAD_STACK_SIZE)),
    _terminate(false),
    _mpe_pid(PID_NULL),
    _replace(false),
    _max_queued(DEFAULT_MAX_QUEUED_SECTION),
    _default_mac(),
    _new_source(),
    _new_dest(),
    _sock(*tsp_),
    _section_queue(DEFAULT_MAX_QUEUED_SECTION),
    _mpe_packets(),
    _packet_index(0),
    _cc_fixer(AllPIDs, tsp)
{
    // UDP receiver common options.
    _sock.defineArgs(*this);

    option(u"mac-address", 0, STRING);
    help(u"mac-address", u"nn:nn:nn:nn:nn:nn",
         u"Specify the default destination MAC address to set in MPE sections for "
         u"unicast IP packets. The default is 00:00:00:00:00:00. For multicast IP "
         u"packets, the MAC address is automatically computed.");

    option(u"max-queue", 0, POSITIVE);
    help(u"max-queue",
         u"Specify the maximum number of queued UDP datagrams before their insertion "
         u"into the MPE stream. The default is " TS_STRINGIFY(DEFAULT_MAX_QUEUED_SECTION) u".");

    option(u"new-destination", 0, STRING);
    help(u"new-destination", u"address[:port]",
         u"Change the destination IP address and UDP port in MPE sections. If the "
         u"port is not specified, the original destination port from the UDP datagram "
         u"is used. By default, the destination address is not modified.");

    option(u"new-source", 0, STRING);
    help(u"new-source", u"address[:port]",
         u"Change the source IP address and UDP port in MPE sections. If the port is "
         u"not specified, the original source port from the UDP datagram is used. By "
         u"default, the source address is not modified.");

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
// Start method
//----------------------------------------------------------------------------

bool ts::MPEInjectPlugin::start()
{
    // Get command line arguments.
    _mpe_pid = intValue<PID>(u"pid");
    _max_queued = intValue<size_t>(u"max-queue", DEFAULT_MAX_QUEUED_SECTION);
    _replace = present(u"replace");
    const UString macAddress(value(u"mac-address"));
    const UString newDestination(value(u"new-destination"));
    const UString newSource(value(u"new-source"));
    if (!_sock.loadArgs(duck, *this)) {
        return false;
    }

    // Decode addresses in command line.
    if ((!macAddress.empty() && !_default_mac.resolve(macAddress, *tsp)) ||
        (!newDestination.empty() && !_new_dest.resolve(newDestination, *tsp)) ||
        (!newSource.empty() && !_new_source.resolve(newSource, *tsp)))
    {
        return false;
    }

    // Create UDP socket
    if (!_sock.open(*tsp)) {
        return false;
    }

    // Reset buffers.
    _section_queue.clear();
    _section_queue.setMaxMessages(_max_queued);
    _mpe_packets.clear();
    _packet_index = 0;
    _cc_fixer.reset();
    _cc_fixer.setGenerator(true);

    // Start the internal thread which listens to incoming UDP packet.
    _terminate = false;
    Thread::start();

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::MPEInjectPlugin::stop()
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

ts::ProcessorPlugin::Status ts::MPEInjectPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Abort if data PID is already present in TS and --replace is not specified.
    const PID pid = pkt.getPID();
    if (!_replace && pid == _mpe_pid) {
        tsp->error(u"MPE PID conflict, specified 0x%X (%d), now found as input PID, try another one", {pid, pid});
        return TSP_END;
    }

    // MPE injection may occur only by replacing null packets or original PID.
    if (pid != PID_NULL && pid != _mpe_pid) {
        // Leave current packet unmodified.
        return TSP_OK;
    }

    // If there is no more TS packet to insert from the previous section, try to get next section.
    // Do not wait for a section, just do nothing if there is none available.
    SectionQueue::MessagePtr section;
    if (_packet_index >= _mpe_packets.size() && _section_queue.dequeue(section, 0) && !section.isNull() && section->isValid()) {
        // Packetize the section.
        OneShotPacketizer zer(duck, _mpe_pid, true);
        zer.addSection(section.changeMutex<NullMutex>());
        zer.getPackets(_mpe_packets);
        _packet_index = 0;
    }

    if (_packet_index < _mpe_packets.size()) {
        // There is an available TS packet, replace the current TS packet.
        pkt = _mpe_packets[_packet_index++];
        _cc_fixer.feedPacket(pkt);
        return TSP_OK;
    }
    else {
        // No available packet, force a null packet.
        return pid == PID_NULL ? TSP_OK : TSP_NULL;
    }
}


//----------------------------------------------------------------------------
// Invoked in the context of the server thread.
//----------------------------------------------------------------------------

void ts::MPEInjectPlugin::main()
{
    tsp->debug(u"server thread started");

    // Try to cumlate "UDP overflow" messages.
    size_t overflow_count = 0;

    size_t insize;
    SocketAddress sender;
    SocketAddress destination;
    ByteBlock buffer(MAX_IP_SIZE);

    // Loop on message reception until a receive error (probably an end of execution).
    while (!_terminate && _sock.receive(buffer.data(), buffer.size(), insize, sender, destination, tsp, *tsp)) {

        // Rebuild source and destination addresses if required.
        if (_new_source.hasAddress()) {
            sender.setAddress(_new_source.address());
        }
        if (_new_source.hasPort()) {
            sender.setPort(_new_source.port());
        }
        if (_new_dest.hasAddress()) {
            destination.setAddress(_new_dest.address());
        }
        if (_new_dest.hasPort()) {
            destination.setPort(_new_dest.port());
        }

        // Compute destination MAC address for MPE section.
        MACAddress mac(_default_mac);
        if (destination.isMulticast()) {
            mac.toMulticast(destination);
        }

        // Create an MPE packet containing this datagram.
        MPEPacket mpe;
        mpe.setSourcePID(_mpe_pid);
        mpe.setSourceSocket(sender);
        mpe.setDestinationSocket(destination);
        mpe.setDestinationMACAddress(mac);
        mpe.setUDPMessage(buffer.data(), insize);

        // Create an MPE section for the datagram.
        SectionQueue::MessagePtr section(new Section());
        mpe.createSection(*section);

        // Enqueue the section immediately. Never wait.
        if (!section->isValid()) {
            tsp->error(u"error creating MPE section from UDP datagram, source: %s, destination: %s, size: %d bytes", {sender, destination, insize});
        }
        else {
            const bool queued = _section_queue.enqueue(section, 0);
            if (!queued) {
                overflow_count++;
            }
            if ((queued && overflow_count > 0) || overflow_count >= OVERFLOW_MSG_GROUP_COUNT) {
                tsp->warning(u"incoming UDP overflow, dropped %d datagrams", {overflow_count});
                overflow_count = 0;
            }
        }
    }

    tsp->debug(u"server thread completed");
}
