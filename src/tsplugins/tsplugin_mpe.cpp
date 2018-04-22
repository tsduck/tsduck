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
//  Extract MPE (Multi-Protocol Encapsulation) datagrams.
//  See ETSI EN 301 192.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsMPEDemux.h"
#include "tsMPEPacket.h"
#include "tsUDPSocket.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class MPEPlugin: public ProcessorPlugin, private MPEHandlerInterface
    {
    public:
        // Implementation of plugin API
        MPEPlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        // Plugin private fields.
        bool          _abort;           // Error, abort asap.
        bool          _log;             // Log MPE datagrams.
        bool          _sync_layout;     // Display a layout of 0x47 sync bytes.
        bool          _send_udp;        // Send all datagrams through UDP.
        UDPSocket     _sock;            // Outgoing UDP socket (forwarded datagrams).
        int           _ttl;             // Time to live option.
        int           _previous_uc_ttl; // Previous unicast TTL which was set.
        int           _previous_mc_ttl; // Previous multicast TTL which was set.
        bool          _all_mpe_pids;    // Extract all MPE PID's.
        PIDSet        _pids;            // Explicitly specified PID's to extract.
        SocketAddress _ip_source;       // IP source filter.
        SocketAddress _ip_dest;         // IP destination filter.
        SocketAddress _ip_forward;      // Forwarded socket address.
        PacketCounter _datagram_count;  // Number of extracted datagrams.
        PacketCounter _max_datagram;    // Maximum number of datagrams to extract.
        bool          _outfile_append;  // Append file.
        UString       _outfile_name;    // Output file name.
        std::ofstream _outfile;         // Output file for extracted datagrams.
        MPEDemux      _demux;           // MPE demux to extract MPE datagrams.

        // Inherited methods.
        virtual void handleMPENewPID(MPEDemux&, const PMT&, PID) override;
        virtual void handleMPEPacket(MPEDemux&, const MPEPacket&) override;

        // Inaccessible operations
        MPEPlugin() = delete;
        MPEPlugin(const MPEPlugin&) = delete;
        MPEPlugin& operator=(const MPEPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(mpe, ts::MPEPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::MPEPlugin::MPEPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Extract MPE (Multi-Protocol Encapsulation) datagrams.", u"[options]"),
    MPEHandlerInterface(),
    _abort(false),
    _log(false),
    _sync_layout(false),
    _send_udp(false),
    _sock(false, *tsp_),
    _ttl(0),
    _previous_uc_ttl(0),
    _previous_mc_ttl(0),
    _all_mpe_pids(false),
    _pids(),
    _ip_source(),
    _ip_dest(),
    _ip_forward(),
    _datagram_count(0),
    _max_datagram(0),
    _outfile_append(false),
    _outfile_name(),
    _outfile(),
    _demux(this)
{
    option(u"append",       'a');
    option(u"destination",  'd', STRING);
    option(u"local-address", 0,  STRING);
    option(u"log",          'l');
    option(u"max-datagram", 'm', POSITIVE);
    option(u"output-file",  'o', STRING);
    option(u"pid",          'p', PIDVAL, 0, UNLIMITED_COUNT);
    option(u"redirect",     'r', STRING);
    option(u"source",       's', STRING);
    option(u"ttl",           0,  INTEGER, 0, 1, 1, 255);
    option(u"sync-layout",   0);
    option(u"udp-forward",  'u');

    setHelp(u"Options:\n"
            u"\n"
            u"  -a\n"
            u"  --append\n"
            u"      With --output-file, if the file already exists, append to the end of the\n"
            u"      file. By default, existing files are overwritten.\n"
            u"\n"
            u"  -d address[:port]\n"
            u"  --destination address[:port]\n"
            u"      Filter MPE UDP datagrams based on the specified destination IP address.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  --local-address address\n"
            u"      With --udp-forward, specify the IP address of the outgoing local interface\n"
            u"      for multicast traffic. It can be also a host name that translates to a\n"
            u"      local address.\n"
            u"\n"
            u"  -l\n"
            u"  --log\n"
            u"      Log all MPE datagrams using a short summary for each of them.\n"
            u"\n"
            u"  -m value\n"
            u"  --max-datagram value\n"
            u"      Specify the maximum number of datagrams to extract, then stop. By default,\n"
            u"      all datagrams are extracted.\n"
            u"\n"
            u"  -o filename\n"
            u"  --output-file filename\n"
            u"      Specify that the extracted UDP datagrams are saved in this file. The UDP\n"
            u"      messages are written without any encapsulation.\n"
            u"\n"
            u"  -p value\n"
            u"  --pid value\n"
            u"      Extract MPE datagrams from this PID. Several -p or --pid options may be\n"
            u"      specified. When no PID is specified, use all PID's carrying MPE which are\n"
            u"      properly declared in the signalization.\n"
            u"\n"
            u"  -r address[:port]\n"
            u"  --redirect address[:port]\n"
            u"      With --udp-forward, redirect all UDP datagrams to the specified socket\n"
            u"      address. By default, all datagram are forwarded to their original\n"
            u"      destination address. If you specify a redirected address, it is\n"
            u"      recommended to use --destination to filter a specific stream. If the\n"
            u"      port is not specified, the original port is used.\n"
            u"\n"
            u"  -s address[:port]\n"
            u"  --source address[:port]\n"
            u"      Filter MPE UDP datagrams based on the specified source IP address.\n"
            u"\n"
            u"  --sync-layout\n"
            u"      With --log, display the layout of 0x47 sync bytes in the UDP payload.\n"
            u"\n"
            u"  --ttl value\n"
            u"      With --udp-forward, specify the TTL (Time-To-Live) socket option.\n"
            u"      The actual option is either \"Unicast TTL\" or \"Multicast TTL\",\n"
            u"      depending on the destination address. By default, use the same TTL\n"
            u"      as specified in the received MPE encapsulated datagram.\n"
            u"\n"
            u"  -u\n"
            u"  --udp-forward\n"
            u"      Forward all received MPE encapsulated UDP datagrams on the local network.\n"
            u"      By default, the destination address and port of each datagram is left\n"
            u"      unchanged. The source address of the forwarded datagrams will be the\n"
            u"      address of the local machine.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::MPEPlugin::start()
{
    // Get command line arguments
    _sync_layout = present(u"sync-layout");
    _log = _sync_layout || present(u"log");
    _send_udp = present(u"udp-forward");
    _outfile_append = present(u"append");
    getValue(_outfile_name, u"output-file");
    getIntValue(_max_datagram, u"max-datagram");
    getIntValue(_ttl, u"ttl");
    getPIDSet(_pids, u"pid");
    const UString ipSource(value(u"source"));
    const UString ipDest(value(u"destination"));
    const UString ipForward(value(u"redirect"));
    const UString ipLocal(value(u"local-address"));

    // Decode socket addresses.
    _ip_source.clear();
    _ip_dest.clear();
    _ip_forward.clear();
    IPAddress localAddress;
    if (!ipSource.empty() && !_ip_source.resolve(ipSource, *tsp)) {
        return false;
    }
    if (!ipDest.empty() && !_ip_dest.resolve(ipDest, *tsp)) {
        return false;
    }
    if (!ipForward.empty() && !_ip_forward.resolve(ipForward, *tsp)) {
        return false;
    }
    if (!ipLocal.empty() && !localAddress.resolve(ipLocal, *tsp)) {
        return false;
    }

    // If no PID is specified, extract all.
    _all_mpe_pids = _pids.none();

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
        // If specified, set TTL option, for unicast and multicast.
        // Otherwise, we will set the TTL for each packet.
        if (_ttl > 0 && (!_sock.setTTL(_ttl, false, *tsp) || !_sock.setTTL(_ttl, true, *tsp))) {
            return false;
        }
        // Specify local address for outgoing multicast traffic.
        if (localAddress.hasAddress() && !_sock.setOutgoingMulticast(localAddress, *tsp)) {
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

    // We will directly access some fields of the IPv4 header.
    assert(mpe.datagramSize() >= IPv4_MIN_HEADER_SIZE);

    // Log MPE packets.
    if (_log) {
        // Get destination IP and MAC address.
        const IPAddress destIP(mpe.destinationIPAddress());
        const MACAddress destMAC(mpe.destinationMACAddress());

        // If the destination IP address is a multicast one, check that the
        // destination MAC address is the correct one.
        MACAddress mcMAC;
        UString macComment;
        if (mcMAC.toMulticast(destIP) && destMAC != mcMAC) {
            macComment = u", should be " + mcMAC.toString();
        }

        // Add a layout of 0x47 sync bytes.
        UString syncBytes;
        const uint8_t* const udp = mpe.udpMessage();
        const size_t udpSize = mpe.udpMessageSize();
        if (_sync_layout) {
            size_t start = 0;
            for (size_t i = 0; i < udpSize; ++i) {
                if (udp[i] == SYNC_BYTE) {
                    if (syncBytes.empty()) {
                        syncBytes = u"\n ";
                    }
                    if (start != i) {
                        syncBytes.append(UString::Format(u" %d", {i - start}));
                    }
                    syncBytes.append(UString::Format(u" [%X]", {SYNC_BYTE}));
                    start = i + 1;
                }
            }
            if (syncBytes.empty()) {
                syncBytes = u" no sync byte";
            }
            else if (start < udpSize) {
                syncBytes.append(UString::Format(u" %d", {udpSize - start}));
            }
        }

        tsp->info(u"PID 0x%X (%d), src: %s:%d, dest: %s:%d (%s%s), %d bytes, fragment: 0x%X%s",
                  {mpe.sourcePID(), mpe.sourcePID(),
                   mpe.sourceIPAddress().toString(), mpe.sourceUDPPort(),
                   destIP.toString(), mpe.destinationUDPPort(),
                   destMAC.toString(), macComment, udpSize,
                   GetUInt16(mpe.datagram() + 6), syncBytes});
    }

    // Save UDP messages in binary file.
    if (_outfile.is_open()) {
        _outfile.write(reinterpret_cast<const char*>(mpe.udpMessage()), std::streamsize(mpe.udpMessageSize()));
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
        SocketAddress dest(mpe.destinationSocket());
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
        if (!_sock.send(mpe.udpMessage(), mpe.udpMessageSize(), dest, *tsp)) {
            _abort = true;
        }
    }

    // Stop after reaching the maximum number of datagrams.
    _datagram_count++;
    if (_max_datagram > 0 && _datagram_count >= _max_datagram) {
        _abort = true;
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::MPEPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Feed the MPE demux.
    _demux.feedPacket(pkt);
    return _abort ? TSP_END : TSP_OK;
}
