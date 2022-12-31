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
//  Pcap and pcap-ng file input.
//
//----------------------------------------------------------------------------

#include "tsAbstractDatagramInputPlugin.h"
#include "tsPluginRepository.h"
#include "tsPcapStream.h"
#include "tsEMMGMUX.h"
#include "tstlvMessageFactory.h"


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
        UString           _file_name;       // Pcap file name.
        IPv4SocketAddress _destination;     // Selected destination UDP socket address.
        IPv4SocketAddress _source;          // Selected source UDP socket address.
        bool              _multicast;       // Use multicast destinations only.
        bool              _udp_emmg_mux;    // Extract packets from EMMG/PDG <=> MUX data provisions in UDP mode.
        bool              _tcp_emmg_mux;    // Extract packets from EMMG/PDG <=> MUX data provisions in TCP mode.
        bool              _has_client_id;   // _emmg_client_id is used.
        bool              _has_data_id;     // _emmg_data_id is used.
        uint32_t          _emmg_client_id;  // EMMG<=>MUX client id to filter.
        uint16_t          _emmg_data_id;    // EMMG<=>MUX data id to filter.

        // Working data:
        PcapFilter           _pcap_udp;         // Pcap file, in UDP mode.
        PcapStream           _pcap_tcp;         // Pcap file, in TCP mode (DVB SimulCrypt EMMG/PDG <=> MUX).
        MicroSecond          _first_tstamp;     // Time stamp of first datagram.
        IPv4SocketAddress    _act_destination;  // Actual destination UDP socket address.
        IPv4SocketAddressSet _all_sources;      // All source addresses.

        // Internal receive methods.
        bool receiveUDP(uint8_t* buffer, size_t buffer_size, size_t& ret_size, MicroSecond& timestamp);
        bool receiveTCP(uint8_t* buffer, size_t buffer_size, size_t& ret_size, MicroSecond& timestamp);

        // Identify and extract TS packets from an EMMG/PDG <=> data_provision message.
        bool isDataProvision(const uint8_t* data, size_t size);
        size_t extractDataProvision(uint8_t* buffer, size_t buffer_size, const uint8_t* msg, size_t msg_size);
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
    _udp_emmg_mux(false),
    _tcp_emmg_mux(false),
    _has_client_id(false),
    _has_data_id(false),
    _emmg_client_id(0),
    _emmg_data_id(0),
    _pcap_udp(),
    _pcap_tcp(),
    _first_tstamp(0),
    _act_destination(),
    _all_sources()
{
    _pcap_udp.defineArgs(*this);

    option(u"", 0, FILENAME, 0, 1);
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

    option(u"emmg-client-id", 0, UINT32);
    help(u"emmg-client-id",
         u"With --tcp-emmg-mux or --udp-emmg-mux, select the EMMG<=>MUX client_id to extract. "
         u"By default, use all client ids.");

    option(u"emmg-data-id", 0, UINT16);
    help(u"emmg-data-id",
         u"With --tcp-emmg-mux or --udp-emmg-mux, select the EMMG<=>MUX data_id to extract. "
         u"By default, use all data ids.");

    option(u"multicast-only", 'm');
    help(u"multicast-only",
         u"When there is no --destination option, select the first multicast address which is found in a UDP datagram. "
         u"By default, use the destination address of the first UDP datagram containing TS packets, unicast or multicast.");

    option(u"source", 's', STRING);
    help(u"source", u"[address][:port]",
         u"Filter UDP datagrams based on the specified source socket address. "
         u"By default, do not filter on source address.");

    option(u"tcp-emmg-mux");
    help(u"tcp-emmg-mux",
         u"Select a TCP stream in the pcap file using the DVB SimulCrypt EMMG/PDG <=> MUX protocol. "
         u"The transport stream is made of the TS packets from the 'data_provision' messages "
         u"(the session must have been set in packet mode, not in section mode). "
         u"This option is typically used to extract EMM PID's as produced by a standard EMMG which feeds a MUX. "
         u"The --source and --destination options define the TCP stream. "
         u"If some address or port are undefined in these two options, the first TCP stream "
         u"matching the specified portions is selected.");

    option(u"udp-emmg-mux");
    help(u"udp-emmg-mux",
         u"Consider each selected UDP datagram as containing a 'data_provision' message "
         u"as defined by the DVB SimulCrypt EMMG/PDG <=> MUX protocol. "
         u"The transport stream is made of the TS packets from these 'data_provision' messages "
         u"(the session must have been set in packet mode, not in section mode). "
         u"This option is typically used to extract EMM PID's as produced by a standard EMMG which feeds a MUX. "
         u"By default, the UDP datagrams contain raw TS packets, with or without RTP headers.");
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
    _udp_emmg_mux = present(u"udp-emmg-mux");
    _tcp_emmg_mux = present(u"tcp-emmg-mux");
    _has_client_id = present(u"emmg-client-id");
    _has_data_id = present(u"emmg-data-id");
    getIntValue(_emmg_client_id, u"emmg-client-id");
    getIntValue(_emmg_data_id, u"emmg-data-id");

    if (_tcp_emmg_mux && _udp_emmg_mux) {
        tsp->error(u"--tcp-emmg-mux and --udp-emmg-mux are mutually exclusive");
        return false;
    }

    // Decode socket addresses.
    _source.clear();
    _destination.clear();
    if (!str_source.empty() && !_source.resolve(str_source, *tsp)) {
        return false;
    }
    if (!str_destination.empty() && !_destination.resolve(str_destination, *tsp)) {
        return false;
    }

    // Get command line arguments for superclass and file filtering options.
    return AbstractDatagramInputPlugin::getOptions() && _pcap_udp.loadArgs(duck, *this) && _pcap_tcp.loadArgs(duck, *this);
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::start()
{
    _first_tstamp = -1;
    _act_destination = _destination;
    _all_sources.clear();

    // Initialize superclass and pcap file.
    bool ok = AbstractDatagramInputPlugin::start();
    if (ok) {
        if (_tcp_emmg_mux) {
            ok = _pcap_tcp.open(_file_name, *tsp);
            if (ok) {
                _pcap_tcp.setBidirectionalFilter(_source, _destination);
                _pcap_tcp.setReportAddressesFilterSeverity(Severity::Verbose);
            }
        }
        else {
            ok = _pcap_udp.open(_file_name, *tsp);
            if (ok) {
                _pcap_udp.setProtocolFilterUDP();
            }
        }
    }
    return ok;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::stop()
{
    _pcap_udp.close();
    _pcap_tcp.close();
    return AbstractDatagramInputPlugin::stop();
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::receiveDatagram(uint8_t* buffer, size_t buffer_size, size_t& ret_size, MicroSecond& timestamp)
{
    return _tcp_emmg_mux ? receiveTCP(buffer, buffer_size, ret_size, timestamp) : receiveUDP(buffer, buffer_size, ret_size, timestamp);
}


//----------------------------------------------------------------------------
// UDP input method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::receiveUDP(uint8_t *buffer, size_t buffer_size, size_t &ret_size, MicroSecond &timestamp)
{
    IPv4Packet ip;

    // Loop on IPv4 datagrams from the pcap file until a matching UDP packet is found (or end of file).
    for (;;) {

        // Read one IPv4 datagram.
        if (!_pcap_udp.readIPv4(ip, timestamp, *tsp)) {
            return 0; // end of file, invalid pcap file format or other i/o error
        }

        // Get IP addresses and UDP ports.
        const IPv4SocketAddress src(ip.sourceSocketAddress());
        const IPv4SocketAddress dst(ip.destinationSocketAddress());

        // Filter source or destination socket address if one was specified.
        if (!src.match(_source) || !dst.match(_act_destination)) {
            continue; // not a matching address
        }

        // If the destination is not yet found, filter multicast addresses if required.
        if (!_act_destination.hasAddress() && _multicast && !dst.isMulticast()) {
            continue; // not a multicast address
        }

        // Locate UDP payload.
        const uint8_t* const udp_data = ip.protocolData();
        const size_t udp_size = ip.protocolDataSize();

        // DVB SimulCrypt vs. raw TS.
        // The destination can be dynamically selected (address, port or both) by the first UDP datagram containing TS packets.
        if (_udp_emmg_mux) {
            // Try to decode UDP packet as DVB SimulCrypt.
            if (!_act_destination.hasAddress() || !_act_destination.hasPort()) {
                // The actual destination is not fully known yet.
                // We are still waiting for the first UDP datagram containing a data_provision message.
                // Is there any in this one?
                if (!isDataProvision(udp_data, udp_size)) {
                    continue; // no data_provision message in this UDP datagram.
                }
                // We just found the first UDP datagram with a data_provision message, now use this destination address all the time.
                _act_destination = dst;
                tsp->verbose(u"using UDP destination address %s", {dst});
            }

            // Extract TS packets from the data_provision message.
            ret_size = extractDataProvision(buffer, buffer_size, udp_data, udp_size);
            if (ret_size == 0) {
                continue; // no TS packets in this message
            }
        }
        else {
            // Look for raw TS.
            if (!_act_destination.hasAddress() || !_act_destination.hasPort()) {
                // The actual destination is not fully known yet.
                // We are still waiting for the first UDP datagram containing TS packets.
                // Is there any TS packet in this one?
                size_t start_index = 0;
                size_t packet_count = 0;
                if (!TSPacket::Locate(ip.protocolData(), ip.protocolDataSize(), start_index, packet_count)) {
                    continue; // no TS packet in this UDP datagram.
                }
                // We just found the first UDP datagram with TS packets, now use this destination address all the time.
                _act_destination = dst;
                tsp->verbose(u"using UDP destination address %s", {dst});
            }

            // Now we have a valid UDP packet.
            ret_size = std::min(ip.protocolDataSize(), buffer_size);
            ::memmove(buffer, ip.protocolData(), ret_size);
        }

        // List all source addresses as they appear.
        if (_all_sources.find(src) == _all_sources.end()) {
            // This is a new source address.
            tsp->verbose(u"%s UDP source address %s", {_all_sources.empty() ? u"using" : u"adding", src});
            _all_sources.insert(src);
        }

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


//----------------------------------------------------------------------------
// TCP input method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::receiveTCP(uint8_t *buffer, size_t buffer_size, size_t &ret_size, MicroSecond &timestamp)
{
    // Read all TCP sessions matching the source and destination until eof or read TS packets.
    ret_size = 0;
    do {
        IPv4SocketAddress source;
        ByteBlock data;

        // Read a message header from any source: version(1), type(2), length(2).
        size_t size = 5;
        if (!_pcap_tcp.readTCP(source, data, size, timestamp, *tsp) || size < 5) {
            return false;
        }
        assert(data.size() == 5);

        // Read the rest of the message from the same source.
        size = GetUInt16(data.data() + 3);
        if (!_pcap_tcp.readTCP(source, data, size, timestamp, *tsp)) {
            return false;
        }

        // Try to extract TS packets from a data_provision message.
        ret_size = extractDataProvision(buffer, buffer_size, data.data(), data.size());

    } while (ret_size == 0);
    return true;
}


//----------------------------------------------------------------------------
// Check if a data area is an EMMG/PDG <=> data_provision message.
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::isDataProvision(const uint8_t* data, size_t size)
{
    // There must be 5 header bytes: version(1), type(2), length(2).
    // See ETSI TS 103 197, section 4.4.1.
    return data != nullptr &&
           size >= 5 &&
           GetUInt16(data + 1) == emmgmux::Tags::data_provision &&
           size >= size_t(5) + GetUInt16(data + 3);
}


//----------------------------------------------------------------------------
// Extract TS packets from an EMMG/PDG <=> data_provision message.
//----------------------------------------------------------------------------

size_t ts::PcapInputPlugin::extractDataProvision(uint8_t* buffer, size_t buffer_size, const uint8_t* msg, size_t msg_size)
{
    // If cannot be a data_provision message, no need to continue.
    if (!isDataProvision(msg, msg_size)) {
        return 0;
    }

    // Adjust protocol version when necessary.
    const ts::tlv::VERSION version = msg[0];
    tlv::Protocol* const protocol = emmgmux::Protocol::Instance();
    if (version != protocol->version()) {
        tsp->debug(u"switching EMMG <=> MUX version protocol to %d", {version});
        protocol->setVersion(version);
    }

    // Interpret the data as data_provision TLV message.
    tlv::MessagePtr ptr;
    tlv::MessageFactory mf(msg, msg_size, protocol);
    if (mf.errorStatus() != tlv::OK) {
        return 0;
    }
    mf.factory(ptr);
    emmgmux::DataProvision* dprov = dynamic_cast<emmgmux::DataProvision*>(ptr.pointer());
    if (dprov == nullptr) {
        return 0;
    }

    // Filter client_id and data_id.
    if ((_has_client_id && dprov->client_id != _emmg_client_id) || (_has_data_id && dprov->data_id != _emmg_data_id)) {
        return 0;
    }

    // Now extract TS packets from the data_provision.
    size_t ret_size = 0;
    for (size_t i = 0; ret_size < buffer_size && i < dprov->datagram.size(); ++i) {
        const ByteBlockPtr& data(dprov->datagram[i]);
        if (!data.isNull() && !data->empty()) {
            if ((*data)[0] != SYNC_BYTE || data->size() % PKT_SIZE != 0) {
                tsp->warning(u"EMMG<=>MUX data_provision not likely TS packets, maybe in section mode");
                return 0;
            }
            const size_t dsize = std::min(buffer_size - ret_size, data->size());
            ::memcpy(buffer + ret_size, data->data(), dsize);
            ret_size += dsize;
        }
    }
    return ret_size;
}
