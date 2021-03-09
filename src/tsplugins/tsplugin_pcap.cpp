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
//
//  Transport stream processor shared library:
//  Pcap and pcap-ng file input.
//
//----------------------------------------------------------------------------

#include "tsAbstractDatagramInputPlugin.h"
#include "tsPluginRepository.h"
#include "tsPcapFile.h"
#include "tsSocketAddress.h"
#include "tsIPUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PcapInputPlugin: public AbstractDatagramInputPlugin
    {
        TS_NOBUILD_NOCOPY(PcapInputPlugin);
    public:
        // Implementation of plugin API
        PcapInputPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;

    protected:
        // Implementation of AbstractDatagramInputPlugin.
        virtual bool receiveDatagram(uint8_t* buffer, size_t buffer_size, size_t& ret_size, MicroSecond& timestamp) override;

    private:
        // Command line options:
        UString       _file_name;    // Pcap file name.
        SocketAddress _destination;  // Selected destination UDP socket address.
        SocketAddress _source;       // Selected source UDP socket address.
        bool          _multicast;    // Use multicast destinations only.

        // Working data:
        PcapFile         _pcap;             // Pcap file processing.
        MicroSecond      _first_tstamp;     // Time stamp of first datagram.
        SocketAddress    _act_destination;  // Actual destination UDP socket address.
        SocketAddressSet _all_sources;      // All source addresses.
    };
}

TS_REGISTER_INPUT_PLUGIN(u"pcap", ts::PcapInputPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PcapInputPlugin::PcapInputPlugin(TSP* tsp_) :
    AbstractDatagramInputPlugin(tsp_, IP_MAX_PACKET_SIZE,
                                u"Read TS packets from a pcap or pcap-ng file", u"[options] [file-name]",
                                u"pcap", u"pcap capture time stamp",
                                false), // not real-time network reception
    _file_name(),
    _destination(),
    _source(),
    _multicast(false),
    _pcap(),
    _first_tstamp(0),
    _act_destination(),
    _all_sources()
{
    option(u"", 0, STRING, 0, 1);
    help(u"", u"file-name",
         u"The name of a '.pcap' or '.pcapng' capture file as produced by Wireshark for instance. "
         u"This input plugin extracts IPv4 UDP datagrams which contain transport stream packets. "
         u"Use the standard input by default, when no file name is specified.");

    option(u"destination", 'd', STRING);
    help(u"destination", u"[address][:port]",
         u"Filter UDP datagrams based on the specified destination socket address. "
         u"By default or if either the IP address or UDP port is missing, "
         u"use the destination of the first matching UDP datagram containing TS packets. "
         u"Then, select only UDP datagrams with this socket address.");

    option(u"multicast-only", 'm');
    help(u"multicast-only",
         u"When there is no --destination option, select the first multicast address which is found in a UDP datagram. "
         u"By default, use the destination address of the first UDP datagram containing TS packets, unicast or multicast.");

    option(u"source", 's', STRING);
    help(u"source", u"[address][:port]",
         u"Filter UDP datagrams based on the specified source socket address. "
         u"By default, do not filter on source address.");
}


//----------------------------------------------------------------------------
// Command line options method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::getOptions()
{
    getValue(_file_name, u"");
    const UString str_source(value(u"source"));
    const UString str_destination(value(u"destination"));
    _multicast = present(u"multicast-only");

    // Decode socket addresses.
    _source.clear();
    _destination.clear();
    if (!str_source.empty() && !_source.resolve(str_source, *tsp)) {
        return false;
    }
    if (!str_destination.empty() && !_destination.resolve(str_destination, *tsp)) {
        return false;
    }

    // Get command line arguments for superclass.
    return AbstractDatagramInputPlugin::getOptions();
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::start()
{
    // Initialize superclass and pcap file.
    _first_tstamp = -1;
    _act_destination = _destination;
    _all_sources.clear();
    return AbstractDatagramInputPlugin::start() && _pcap.open(_file_name, *tsp);
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::stop()
{
    _pcap.close();
    return AbstractDatagramInputPlugin::stop();
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::receiveDatagram(uint8_t* buffer, size_t buffer_size, size_t& ret_size, MicroSecond& timestamp)
{
    // Loop on IPv4 datagrams from the pcap file until a matching UDP packet is found (or end of file).
    for (;;) {

        // Read one IPv4 datagram.
        if (!_pcap.readIPv4(buffer, buffer_size, ret_size, timestamp, *tsp)) {
            return 0; // end of file, invalid pcap file format or other i/o error
        }

        // Check that this looks like an IPv4 packet with a UDP transport.
        const size_t ip_header_size = IPHeaderSize(buffer, ret_size);
        if (ip_header_size == 0 || ret_size < ip_header_size + UDP_HEADER_SIZE) {
            continue; // not valid IP + UDP headers.
        }

        // Total size of UDP packet, including header.
        size_t udp_length = GetUInt16BE(buffer + ip_header_size + UDP_LENGTH_OFFSET);
        if (udp_length < UDP_HEADER_SIZE || ret_size < ip_header_size + udp_length) {
            continue; // truncated UDP packet.
        }

        // Address and length of UDP payload.
        const uint8_t* udp_payload = buffer + ip_header_size + UDP_HEADER_SIZE;
        udp_length -= UDP_HEADER_SIZE;

        // Get IP addresses and UDP ports.
        const SocketAddress src(GetUInt32(buffer + IPv4_SRC_ADDR_OFFSET), GetUInt16(buffer + ip_header_size + UDP_SRC_PORT_OFFSET));
        const SocketAddress dst(GetUInt32(buffer + IPv4_DEST_ADDR_OFFSET), GetUInt16(buffer + ip_header_size + UDP_DEST_PORT_OFFSET));

        // Filter source or destination socket address if one was specified.
        if (!src.match(_source) || !dst.match(_act_destination)) {
            continue; // not a matching address
        }

        // If the destination is not yet found, filter multicast addresses if required.
        if (!_act_destination.hasAddress() && _multicast && !dst.isMulticast()) {
            continue; // not a multicast address
        }

        // The destination can be dynamically selected (address, port or both) by the first UDP datagram containing TS packets.
        if (!_act_destination.hasAddress() || !_act_destination.hasPort()) {
            // The actual destination is not fully known yet.
            // We are still waiting for the first UDP datagram containing TS packets.
            // Is there any TS packet in this one?
            size_t start_index = 0;
            size_t packet_count = 0;
            if (!TSPacket::Locate(udp_payload, udp_length, start_index, packet_count)) {
                continue; // no TS packet in this UDP datagram.
            }
            // We just found the first UDP datagram with TS packets, now use this destination address all the time.
            _act_destination = dst;
            tsp->verbose(u"using UDP destination address %s", {dst});
        }

        // List all source addresses as they appear.
        if (_all_sources.find(src) == _all_sources.end()) {
            // This is a new source address.
            tsp->verbose(u"%s UDP source address %s", {_all_sources.empty() ? u"using" : u"adding", src});
            _all_sources.insert(src);
        }

        // Now we have a valid UDP packet. Pack the returned data to remove the IP and UDP headers.
        // Note that memmove() supports overlapping source and destination.
        ::memmove(buffer, udp_payload, udp_length);
        ret_size = udp_length;

        // Adjust time stamps according to first one.
        if (timestamp >= 0) {
            if (_first_tstamp < 0) {
                // This is the first time stamp, the origin.
                _first_tstamp = timestamp;
                timestamp = 0;
            }
            else {
                // Return a relative value from first timestamp.
                timestamp -= _first_tstamp;
            }
        }

        // Return a valid UDP payload.
        return true;
    }
}
