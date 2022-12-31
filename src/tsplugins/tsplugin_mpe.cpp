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
//  Extract MPE (Multi-Protocol Encapsulation) datagrams.
//  See ETSI EN 301 192.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsPluginEventData.h"
#include "tsMPEDemux.h"
#include "tsMPEPacket.h"
#include "tsUDPSocket.h"
#include "tsIPProtocols.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class MPEPlugin: public ProcessorPlugin, private MPEHandlerInterface
    {
        TS_NOBUILD_NOCOPY(MPEPlugin);
    public:
        // Implementation of plugin API
        MPEPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options.
        bool          _log;               // Log MPE datagrams.
        bool          _sync_layout;       // Display a layout of 0x47 sync bytes.
        bool          _dump_datagram;     // Dump complete network datagrams.
        bool          _dump_udp;          // Dump UDP payloads.
        bool          _send_udp;          // Send all datagrams through UDP.
        bool          _log_hexa_line;     // Log datagrams as one hexa line in the system message log.
        bool          _signal_event;      // Signal a plugin event on MPE packet.
        bool          _all_mpe_pids;      // Extract all MPE PID's.
        bool          _outfile_append;    // Append file.
        UString       _outfile_name;      // Output file name.
        UString       _log_hexa_prefix;   // Prefix before hexa log line.
        PacketCounter _max_datagram;      // Maximum number of datagrams to extract.
        size_t        _min_net_size;      // Minimum size of network datagrams.
        size_t        _max_net_size;      // Maximum size of network datagrams.
        size_t        _min_udp_size;      // Minimum size of UDP datagrams.
        size_t        _max_udp_size;      // Maximum size of UDP datagrams.
        size_t        _dump_max;          // Max dump size in bytes.
        size_t        _skip_size;         // Initial bytes to skip for --dump and --output-file.
        uint32_t      _event_code;        // Event code to signal.
        int           _ttl;               // Time to live option.
        PIDSet        _pids;              // Explicitly specified PID's to extract.
        IPv4SocketAddress _ip_source;     // IP source filter.
        IPv4SocketAddress _ip_dest;       // IP destination filter.
        IPv4SocketAddress _ip_forward;    // Forwarded socket address.
        IPv4Address       _local_address; // Local IP address for UDP forwarding.
        uint16_t          _local_port;    // Local UDP source port for UDP forwarding.

        // Plugin private fields.
        bool          _abort;             // Error, abort asap.
        UDPSocket     _sock;              // Outgoing UDP socket (forwarded datagrams).
        int           _previous_uc_ttl;   // Previous unicast TTL which was set.
        int           _previous_mc_ttl;   // Previous multicast TTL which was set.
        PacketCounter _datagram_count;    // Number of extracted datagrams.
        std::ofstream _outfile;           // Output file for extracted datagrams.
        MPEDemux      _demux;             // MPE demux to extract MPE datagrams.

        // Inherited methods.
        virtual void handleMPENewPID(MPEDemux&, const PMT&, PID) override;
        virtual void handleMPEPacket(MPEDemux&, const MPEPacket&) override;

        // Build the string for --sync-layout.
        UString syncLayoutString(const uint8_t* udp, size_t udpSize);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"mpe", ts::MPEPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::MPEPlugin::MPEPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Extract MPE (Multi-Protocol Encapsulation) datagrams", u"[options]"),
    MPEHandlerInterface(),
    _log(false),
    _sync_layout(false),
    _dump_datagram(false),
    _dump_udp(false),
    _send_udp(false),
    _log_hexa_line(false),
    _signal_event(false),
    _all_mpe_pids(false),
    _outfile_append(false),
    _outfile_name(),
    _log_hexa_prefix(),
    _max_datagram(0),
    _min_net_size(0),
    _max_net_size(0),
    _min_udp_size(0),
    _max_udp_size(0),
    _dump_max(0),
    _skip_size(0),
    _event_code(0),
    _ttl(0),
    _pids(),
    _ip_source(),
    _ip_dest(),
    _ip_forward(),
    _local_address(),
    _local_port(IPv4SocketAddress::AnyPort),
    _abort(false),
    _sock(false, *tsp_),
    _previous_uc_ttl(0),
    _previous_mc_ttl(0),
    _datagram_count(0),
    _outfile(),
    _demux(duck, this)
{
    option(u"append", 'a');
    help(u"append",
         u"With --output-file, if the file already exists, append to the end of the "
         u"file. By default, existing files are overwritten.");

    option(u"destination", 'd', STRING);
    help(u"destination", u"address[:port]",
         u"Filter MPE UDP datagrams based on the specified destination IP address.");

    option(u"dump-datagram");
    help(u"dump-datagram", u"With --log or --log-hexa-line, dump each complete network datagram.");

    option(u"dump-udp");
    help(u"dump-udp", u"With --log or --log-hexa-line, dump the UDP payload of each network datagram.");

    option(u"dump-max", 0, UNSIGNED);
    help(u"dump-max",
         u"With --dump-datagram, --dump-udp or --log-hexa-line, specify the maximum number of bytes to dump. "
         u"By default, dump everything.");

    option(u"event-code", 0, UINT32);
    help(u"event-code",
         u"This option is for C++, Java or Python developers only.\n\n"
         u"Signal a plugin event with the specified code for each MPE packet. "
         u"The event data is an instance of PluginEventData pointing to "
         u"the network datagram (default) or UDP payload (with --dump-udp).");

    option(u"local-address", 0, STRING);
    help(u"local-address", u"address",
         u"With --udp-forward, specify the IP address of the outgoing local interface "
         u"for multicast traffic. It can be also a host name that translates to a "
         u"local address.");

    option(u"local-port", 0, UINT16);
    help(u"local-port",
         u"With --udp-forward, specify the local UDP source port for outgoing packets. "
         u"By default, a random source port is used.");

    option(u"net-size", 0, UNSIGNED);
    help(u"net-size",
         u"Specify the exact size in bytes of the network datagrams to filter. "
         u"This option is incompatible with --min-net-size and --max-net-size.");

    option(u"min-net-size", 0, UNSIGNED);
    help(u"min-net-size", u"Specify the minimum size in bytes of the network datagrams to filter.");

    option(u"max-net-size", 0, UNSIGNED);
    help(u"max-net-size", u"Specify the maximum size in bytes of the network datagrams to filter.");

    option(u"log", 'l');
    help(u"log",
         u"Log all MPE datagrams using a short summary for each of them.");

    option(u"log-hexa-line", 0, Args::STRING, 0, 1, 0, Args::UNLIMITED_VALUE, true);
    help(u"log-hexa-line", u"'prefix'",
         u"Same as --log but log the full content of each datagram as one single hexadecimal line in the message logger. "
         u"Dump either the full datagram (the default) or the UDP payload (with --dump-udp). "
         u"The optional string parameter specifies a prefix to prepend on the log "
         u"line before the hexadecimal text to locate the appropriate line in the logs.");

    option(u"max-datagram", 'm', POSITIVE);
    help(u"max-datagram",
         u"Specify the maximum number of datagrams to extract, then stop. By default, "
         u"all datagrams are extracted.");

    option(u"output-file", 'o', FILENAME);
    help(u"output-file", u"filename",
         u"Specify that the extracted UDP datagrams are saved in this file. The UDP "
         u"messages are written without any encapsulation.");

    option(u"pid", 'p', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Extract MPE datagrams from these PID's. Several -p or --pid options may be "
         u"specified. When no PID is specified, use all PID's carrying MPE which are "
         u"properly declared in the signalization.");

    option(u"redirect", 'r', STRING);
    help(u"redirect", u"address[:port]",
         u"With --udp-forward, redirect all UDP datagrams to the specified socket "
         u"address. By default, all datagrams are forwarded to their original "
         u"destination address. If you specify a redirected address, it is "
         u"recommended to use --destination to filter a specific stream. If the "
         u"port is not specified, the original port is used.");

    option(u"skip", 0, UNSIGNED);
    help(u"skip",
         u"With --output-file, --dump-datagram, --dump-udp or --log-hexa-line, specify the initial "
         u"number of bytes to skip. By default, save or dump from the beginning.");

    option(u"source", 's', STRING);
    help(u"source", u"address[:port]",
         u"Filter MPE UDP datagrams based on the specified source IP address.");

    option(u"sync-layout");
    help(u"sync-layout",
         u"With --log, display the layout of 0x47 sync bytes in the UDP payload.");

    option(u"ttl", 0, INTEGER, 0, 1, 1, 255);
    help(u"ttl",
         u"With --udp-forward, specify the TTL (Time-To-Live) socket option. "
         u"The actual option is either \"Unicast TTL\" or \"Multicast TTL\", "
         u"depending on the destination address. By default, use the same TTL "
         u"as specified in the received MPE encapsulated datagram.");

    option(u"udp-forward", 'u');
    help(u"udp-forward",
         u"Forward all received MPE encapsulated UDP datagrams on the local network. "
         u"By default, the destination address and port of each datagram is left "
         u"unchanged. The source address of the forwarded datagrams will be the "
         u"address of the local machine.");

    option(u"udp-size", 0, UNSIGNED);
    help(u"udp-size",
        u"Specify the exact size in bytes of the UDP datagrams to filter. "
        u"This option is incompatible with --min-udp-size and --max-udp-size.");

    option(u"min-udp-size", 0, UNSIGNED);
    help(u"min-udp-size", u"Specify the minimum size in bytes of the UDP datagrams to filter.");

    option(u"max-udp-size", 0, UNSIGNED);
    help(u"max-udp-size", u"Specify the maximum size in bytes of the UDP datagrams to filter.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::MPEPlugin::getOptions()
{
    // Get command line arguments
    _sync_layout = present(u"sync-layout");
    _dump_datagram = present(u"dump-datagram");
    _dump_udp = present(u"dump-udp");
    _log_hexa_line = present(u"log-hexa-line");
    _signal_event = present(u"event-code");
    _log = _sync_layout || (_dump_udp && !_signal_event) || _dump_datagram || _log_hexa_line || present(u"log");
    _send_udp = present(u"udp-forward");
    _outfile_append = present(u"append");
    getValue(_outfile_name, u"output-file");
    getValue(_log_hexa_prefix, u"log-hexa-line");
    getIntValue(_max_datagram, u"max-datagram");
    getIntValue(_dump_max, u"dump-max", NPOS);
    getIntValue(_skip_size, u"skip");
    getIntValue(_event_code, u"event-code");
    getIntValue(_ttl, u"ttl");
    getIntValues(_pids, u"pid");
    const UString ipSource(value(u"source"));
    const UString ipDest(value(u"destination"));
    const UString ipForward(value(u"redirect"));
    const UString ipLocal(value(u"local-address"));
    getIntValue(_local_port, u"local-port", IPv4SocketAddress::AnyPort);
    getIntValue(_min_net_size, u"min-net-size");
    getIntValue(_max_net_size, u"max-net-size", NPOS);
    getIntValue(_min_udp_size, u"min-udp-size");
    getIntValue(_max_udp_size, u"max-udp-size", NPOS);

    // --net-size N is a shortcut for --min-net-size N --max-net-size N.
    if (present(u"net-size")) {
        if (present(u"min-net-size") || present(u"max-net-size")) {
            tsp->error(u"--net-size is incompatible with --min-net-size and --max-net-size");
            return false;
        }
        else {
            _min_net_size = _max_net_size = intValue<size_t>(u"net-size");
        }
    }

    // --udp-size N is a shortcut for --min-udp-size N --max-udp-size N.
    if (present(u"udp-size")) {
        if (present(u"min-udp-size") || present(u"max-udp-size")) {
            tsp->error(u"--udp-size is incompatible with --min-udp-size and --max-udp-size");
            return false;
        }
        else {
            _min_udp_size = _max_udp_size = intValue<size_t>(u"udp-size");
        }
    }

    // Decode socket addresses.
    _ip_source.clear();
    _ip_dest.clear();
    _ip_forward.clear();
    _local_address.clear();
    if (!ipSource.empty() && !_ip_source.resolve(ipSource, *tsp)) {
        return false;
    }
    if (!ipDest.empty() && !_ip_dest.resolve(ipDest, *tsp)) {
        return false;
    }
    if (!ipForward.empty() && !_ip_forward.resolve(ipForward, *tsp)) {
        return false;
    }
    if (!ipLocal.empty() && !_local_address.resolve(ipLocal, *tsp)) {
        return false;
    }

    // If no PID is specified, extract all.
    _all_mpe_pids = _pids.none();
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::MPEPlugin::start()
{
    // Initialize the MPE demux.
    _demux.reset();
    _demux.addPIDs(_pids);

    // Open/create output file if present.
    if (!_outfile_name.empty()) {
        std::ios::openmode mode(std::ios::out | std::ios::binary);
        if (_outfile_append) {
            mode |= std::ios::ate;
        }
        _outfile.open(_outfile_name.toUTF8().c_str(), mode);
        if (!_outfile.is_open()) {
            tsp->error(u"error creating %s", {_outfile_name});
            return false;
        }
    }

    // Initialize the forwarding UDP socket.
    if (_send_udp) {
        if (!_sock.open(*tsp)) {
            return false;
        }
        // If local port is specified, bint to socket.
        const IPv4SocketAddress local(IPv4Address::AnyAddress, _local_port);
        if (_local_port != IPv4SocketAddress::AnyPort && (!_sock.reusePort(true, *tsp) || !_sock.bind(local, *tsp))) {
            return false;
        }
        // If specified, set TTL option, for unicast and multicast.
        // Otherwise, we will set the TTL for each packet.
        if (_ttl > 0 && (!_sock.setTTL(_ttl, false, *tsp) || !_sock.setTTL(_ttl, true, *tsp))) {
            return false;
        }
        // Specify local address for outgoing multicast traffic.
        if (_local_address.hasAddress() && !_sock.setOutgoingMulticast(_local_address, *tsp)) {
            return false;
        }
    }

    // Other states.
    _datagram_count = 0;
    _previous_uc_ttl = _previous_mc_ttl = 0;

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::MPEPlugin::stop()
{
    // Close output file.
    if (_outfile.is_open()) {
        _outfile.close();
    }

    // Close the forwarding socket.
    if (_sock.isOpen()) {
        _sock.close(*tsp);
    }

    return true;
}


//----------------------------------------------------------------------------
// Process new MPE PID.
//----------------------------------------------------------------------------

void ts::MPEPlugin::handleMPENewPID(MPEDemux& demux, const PMT& pmt, PID pid)
{
    // Found a new PID carrying MPE.
    // If we need to extract all MPE PID's, add it.
    if (_all_mpe_pids) {
        tsp->verbose(u"extract new MPE PID 0x%X (%d), service 0x%X (%d)", {pid, pid, pmt.service_id, pmt.service_id});
        _demux.addPID(pid);
    }
}


//----------------------------------------------------------------------------
// Process a MPE packet.
//----------------------------------------------------------------------------

void ts::MPEPlugin::handleMPEPacket(MPEDemux& demux, const MPEPacket& mpe)
{
    // If already aborting, do nothing.
    if (_abort) {
        return;
    }

    // Apply source and destination filters.
    if (!mpe.sourceSocket().match(_ip_source) || !mpe.destinationSocket().match(_ip_dest)) {
        return;
    }

    // Network datagram and UDP payload.
    const uint8_t* const net_data = mpe.datagram();
    const uint8_t* const udp_data = mpe.udpMessage();
    const size_t udp_size = mpe.udpMessageSize();
    const size_t net_size = mpe.datagramSize();

    // Apply size filters.
    if (net_size < _min_net_size || net_size > _max_net_size || udp_size < _min_udp_size || udp_size > _max_udp_size) {
        return;
    }

    // We will directly access some fields of the IPv4 header.
    assert(net_size >= IPv4_MIN_HEADER_SIZE);

    // Select what to dump.
    const uint8_t* dump_data = _dump_udp ? udp_data : net_data;
    size_t dump_size = _dump_udp ? udp_size : net_size;

    // Skip initial bytes.
    if (dump_size <= _skip_size) {
        dump_size = 0;
    }
    else {
        dump_data += _skip_size;
        dump_size -= _skip_size;
    }

    // Maximum dump size.
    dump_size = std::min(dump_size, _dump_max);

    // Log MPE packets.
    if (_log_hexa_line) {
        tsp->info(_log_hexa_prefix + UString::Dump(dump_data, dump_size, UString::COMPACT));
    }
    else if (_log) {
        // Get destination IP and MAC address.
        const IPv4Address destIP(mpe.destinationIPAddress());
        const MACAddress destMAC(mpe.destinationMACAddress());

        // If the destination IP address is a multicast one, check that the
        // destination MAC address is the correct one.
        MACAddress mcMAC;
        UString macComment;
        if (mcMAC.toMulticast(destIP) && destMAC != mcMAC) {
            macComment = u", should be " + mcMAC.toString();
        }

        // Finally log the complete message.
        UString dump;
        if (dump_size > 0) {
            dump.append(u"\n");
            dump.appendDump(dump_data, dump_size, UString::HEXA | UString::ASCII | UString::OFFSET | UString::BPL, 2, 16);
        }
        tsp->info(u"PID 0x%X (%<d), src: %s:%d, dest: %s:%d (%s%s), %d bytes, fragment: 0x%X%s%s",
                  {mpe.sourcePID(), mpe.sourceIPAddress(), mpe.sourceUDPPort(),
                   destIP, mpe.destinationUDPPort(), destMAC, macComment, udp_size,
                   GetUInt16(mpe.datagram() + 6), syncLayoutString(udp_data, udp_size),
                   dump});
    }

    // Save UDP messages in binary file.
    if (_outfile.is_open() && udp_size > _skip_size) {
        _outfile.write(reinterpret_cast<const char*>(udp_data + _skip_size), std::streamsize(udp_size - _skip_size));
        if (!_outfile) {
            tsp->error(u"error writing to %s", {_outfile_name});
            _abort = true;
        }
    }

    // Forward UDP datagrams.
    if (_send_udp) {

        // Determine the destination address.
        // Start with original address for MPE section.
        // Then override with user-specified values.
        IPv4SocketAddress dest(mpe.destinationSocket());
        if (_ip_forward.hasAddress()) {
            dest.setAddress(_ip_forward.address());
        }
        if (_ip_forward.hasPort()) {
            dest.setPort(_ip_forward.port());
        }

        // Set the TTL from the datagram is not already set by user-specified value.
        const bool mc = dest.isMulticast();
        const int previous_ttl = mc ? _previous_mc_ttl : _previous_uc_ttl;
        const int mpe_ttl = mpe.datagram()[8]; // in original IP header
        if (_ttl <= 0 && mpe_ttl != previous_ttl && _sock.setTTL(mpe_ttl, mc, *tsp)) {
            if (mc) {
                _previous_mc_ttl = mpe_ttl;
            }
            else {
                _previous_uc_ttl = mpe_ttl;
            }
        }

        // Send the UDP datagram.
        if (!_sock.send(udp_data, udp_size, dest, *tsp)) {
            _abort = true;
        }
    }

    // Signal application-defined event. The call to the application callbacks is synchronous.
    if (_signal_event) {
        PluginEventData data(dump_data, dump_size);
        tsp->signalPluginEvent(_event_code, &data);
    }

    // Stop after reaching the maximum number of datagrams.
    _datagram_count++;
    if (_max_datagram > 0 && _datagram_count >= _max_datagram) {
        _abort = true;
    }
}


//----------------------------------------------------------------------------
// Build the string for --sync-layout.
//----------------------------------------------------------------------------

ts::UString ts::MPEPlugin::syncLayoutString(const uint8_t* udp, size_t udp_size)
{
    // Nothing to display without --sync-layout.
    if (!_sync_layout) {
        return UString();
    }

    // Build list of indexs of 0x47 sync bytes.
    std::vector<size_t> syncIndex;

    // Check is we find sync bytes with shorter distances than 187 bytes.
    bool hasShorter = false;

    // Build the log string.
    UString result;
    size_t start = 0;
    for (size_t i = 0; i < udp_size; ++i) {
        if (udp[i] == SYNC_BYTE) {
            syncIndex.push_back(i);
            hasShorter = hasShorter || i - start < PKT_SIZE - 1;
            if (result.empty()) {
                result = u"\n ";
            }
            if (i > start) {
                result.append(UString::Format(u" %d", {i - start}));
            }
            result.append(u" S");
            start = i + 1;
        }
    }
    if (result.empty()) {
        return u"\n  no sync byte";
    }
    else if (start < udp_size) {
        result.append(UString::Format(u" %d", {udp_size - start}));
    }

    // If we have shorter intervals (less than 187), maybe some 0x47 were simply data bytes.
    // Try to find complete TS packets, starting at first 0x47, then second, etc.
    if (hasShorter) {
        // Loop on starting 0x47 from the previous list.
        for (size_t si = 0; si < syncIndex.size() && syncIndex[si] + PKT_SIZE <= udp_size; ++si) {
            // Check if we can find complete TS packets starting here.
            bool found = true;
            for (size_t i = syncIndex[si]; found && i < udp_size; i += PKT_SIZE) {
                found = udp[i] == SYNC_BYTE;
            }
            if (found) {
                // Yes, found a list of complete TS packets.
                result.append(UString::Format(u"\n  %d", {syncIndex[si]}));
                for (size_t i = syncIndex[si]; i < udp_size; i += PKT_SIZE) {
                    result.append(UString::Format(u" S %d", {std::min(PKT_SIZE - 1, udp_size - i)}));
                }
                // Not need to try starting at the next sync byte?
                break;
            }
        }
    }

    return result;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::MPEPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Feed the MPE demux.
    _demux.feedPacket(pkt);
    return _abort ? TSP_END : TSP_OK;
}
