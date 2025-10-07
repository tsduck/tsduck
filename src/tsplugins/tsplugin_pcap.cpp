//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        TS_PLUGIN_CONSTRUCTORS(PcapInputPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;

    protected:
        // Implementation of AbstractDatagramInputPlugin.
        virtual bool receiveDatagram(uint8_t* buffer, size_t buffer_size, size_t& ret_size, cn::microseconds& timestamp, TimeSource& timesource) override;

    private:
        // Command line options:
        fs::path        _file_name {};            // Pcap file name.
        IPSocketAddress _destination {};          // Selected destination UDP socket address.
        IPSocketAddress _source {};               // Selected source UDP socket address.
        bool            _multicast = false;       // Use multicast destinations only.
        bool            _http = false;            // Extract packets from an HTTP session.
        bool            _udp_emmg_mux = false;    // Extract packets from EMMG/PDG <=> MUX data provisions in UDP mode.
        bool            _tcp_emmg_mux = false;    // Extract packets from EMMG/PDG <=> MUX data provisions in TCP mode.
        bool            _has_client_id = false;   // _emmg_client_id is used.
        bool            _has_data_id = false;     // _emmg_data_id is used.
        uint32_t        _emmg_client_id = 0;      // EMMG<=>MUX client id to filter.
        uint16_t        _emmg_data_id = 0;        // EMMG<=>MUX data id to filter.
        size_t          _http_chunk_size = 65535; // Size to load from the TCP session each time we reload the buffer.

        // Working data:
        PcapFilter         _pcap_udp {};          // Pcap file, in UDP mode.
        PcapStream         _pcap_tcp {};          // Pcap file, in TCP mode (DVB SimulCrypt EMMG/PDG <=> MUX).
        cn::microseconds   _first_tstamp {};      // Time stamp of first datagram.
        IPSocketAddress    _actual_dest {};       // Actual destination UDP socket address.
        IPSocketAddress    _actual_source {};     // Actual source TCP socket address for HTTP mode.
        IPSocketAddressSet _all_sources {};       // All source addresses.
        emmgmux::Protocol  _emmgmux {};           // EMMG/PDG <=> MUX protocol instance to decode TCP stream.
        ByteBlock          _data {};              // Session data buffer, for HTTP mode.
        size_t             _data_next = 0;        // Next index in _data.
        bool               _data_error = false;   // Content of _data is invalid.
        bool (PcapInputPlugin::*_receive)(uint8_t*, size_t, size_t&, cn::microseconds&) = nullptr; // Receive handler.

        // Internal receive methods.
        bool receiveUDP(uint8_t* buffer, size_t buffer_size, size_t& ret_size, cn::microseconds& timestamp);
        bool receiveEMMG(uint8_t* buffer, size_t buffer_size, size_t& ret_size, cn::microseconds& timestamp);
        bool receiveHTTP(uint8_t* buffer, size_t buffer_size, size_t& ret_size, cn::microseconds& timestamp);

        // Identify and extract TS packets from an EMMG/PDG <=> MUX data_provision message.
        bool isDataProvision(const uint8_t* data, size_t size);
        size_t extractDataProvision(uint8_t* buffer, size_t buffer_size, const uint8_t* msg, size_t msg_size);

        // Report an HTTP content error, make the rest of the stream as invalid.
        void contentErrorHTTP();
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
                                TSDatagramInputOptions::ALLOW_RS204)
{
    _pcap_udp.defineArgs(*this);

    option(u"", 0, FILENAME, 0, 1);
    help(u"", u"file-name",
         u"The name of a '.pcap' or '.pcapng' capture file as produced by Wireshark for instance. "
         u"This input plugin extracts IPv4 UDP datagrams which contain transport stream packets. "
         u"Use the standard input by default, when no file name is specified.");

    option(u"destination", 'd', IPSOCKADDR_OAP);
    help(u"destination",
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

    option(u"http", 'h');
    help(u"http",
         u"Select a TCP stream in the pcap file using the HTTP protocol and extract TS packets from the response. "
         u"The --source and --destination options define the TCP stream. "
         u"If some address or port are undefined in these two options, the first TCP stream "
         u"matching the specified portions is selected.");

    option(u"multicast-only", 'm');
    help(u"multicast-only",
         u"When there is no --destination option, select the first multicast address which is found in a UDP datagram. "
         u"By default, use the destination address of the first UDP datagram containing TS packets, unicast or multicast.");

    option(u"source", 's', IPSOCKADDR_OAP);
    help(u"source",
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
    getPathValue(_file_name, u"");
    getSocketValue(_source, u"source");
    getSocketValue(_destination, u"destination");
    _multicast = present(u"multicast-only");
    _http = present(u"http");
    _udp_emmg_mux = present(u"udp-emmg-mux");
    _tcp_emmg_mux = present(u"tcp-emmg-mux");
    _has_client_id = present(u"emmg-client-id");
    _has_data_id = present(u"emmg-data-id");
    getIntValue(_emmg_client_id, u"emmg-client-id");
    getIntValue(_emmg_data_id, u"emmg-data-id");

    if (_http + _tcp_emmg_mux + _udp_emmg_mux > 1) {
        error(u"--http, --tcp-emmg-mux, --udp-emmg-mux are mutually exclusive");
        return false;
    }
    if (_http && !_source.hasAddress() && !_destination.hasAddress()) {
        error(u"--http requires at least --source or --destination");
        return false;
    }

    // Get command line arguments for superclass and file filtering options.
    return AbstractDatagramInputPlugin::getOptions() && _pcap_udp.loadArgs(*this) && _pcap_tcp.loadArgs(*this);
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::start()
{
    _first_tstamp = cn::microseconds(-1);
    _actual_dest = _destination;
    _actual_source = _source;
    _all_sources.clear();
    _data.clear();
    _data_next = 0;
    _data_error = false;

    // Select the right receive method.
    if (_http) {
        _receive = &PcapInputPlugin::receiveHTTP;
        setDatagram(false);
    }
    else if (_tcp_emmg_mux) {
        _receive = &PcapInputPlugin::receiveEMMG;
        setDatagram(false);
    }
    else {
        _receive = &PcapInputPlugin::receiveUDP;
        setDatagram(true);
    }

    // Initialize superclass and pcap file.
    bool ok = AbstractDatagramInputPlugin::start();
    if (ok) {
        if (_http || _tcp_emmg_mux) {
            ok = _pcap_tcp.open(_file_name, *this);
            _pcap_tcp.setBidirectionalFilter(_source, _destination);
            _pcap_tcp.setReportAddressesFilterSeverity(Severity::Verbose);
        }
        else {
            ok = _pcap_udp.open(_file_name, *this);
            _pcap_udp.setProtocolFilterUDP();
        }
    }
    return ok;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::stop()
{
    const size_t max = _pcap_tcp.maxReassemblyQueueSize();
    if (max > 0) {
        debug(u"max TCP reassembly queue size: %d data blocks", max);
    }
    _pcap_udp.close();
    _pcap_tcp.close();
    return AbstractDatagramInputPlugin::stop();
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::receiveDatagram(uint8_t* buffer, size_t buffer_size, size_t& ret_size, cn::microseconds& timestamp, TimeSource& timesource)
{
    // Dispatch on appropriate receive handler.
    timesource = TimeSource::PCAP;
    return (this->*_receive)(buffer, buffer_size, ret_size, timestamp);
}


//----------------------------------------------------------------------------
// UDP input method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::receiveUDP(uint8_t *buffer, size_t buffer_size, size_t &ret_size, cn::microseconds &timestamp)
{
    IPPacket ip;
    VLANIdStack vlans;

    // Loop on IPv4 datagrams from the pcap file until a matching UDP packet is found (or end of file).
    for (;;) {

        // Read one IPv4 datagram.
        if (!_pcap_udp.readIP(ip, vlans, timestamp, *this)) {
            return 0; // end of file, invalid pcap file format or other i/o error
        }

        // Get IP addresses and UDP ports.
        const IPSocketAddress src(ip.source());
        const IPSocketAddress dst(ip.destination());

        // Filter source or destination socket address if one was specified.
        if (!src.match(_source) || !dst.match(_actual_dest)) {
            continue; // not a matching address
        }

        // If the destination is not yet found, filter multicast addresses if required.
        if (!_actual_dest.hasAddress() && _multicast && !dst.isMulticast()) {
            continue; // not a multicast address
        }

        // Locate UDP payload.
        const uint8_t* const udp_data = ip.protocolData();
        const size_t udp_size = ip.protocolDataSize();

        // DVB SimulCrypt vs. raw TS.
        // The destination can be dynamically selected (address, port or both) by the first UDP datagram containing TS packets.
        if (_udp_emmg_mux) {
            // Try to decode UDP packet as DVB SimulCrypt.
            if (!_actual_dest.hasAddress() || !_actual_dest.hasPort()) {
                // The actual destination is not fully known yet.
                // We are still waiting for the first UDP datagram containing a data_provision message.
                // Is there any in this one?
                if (!isDataProvision(udp_data, udp_size)) {
                    continue; // no data_provision message in this UDP datagram.
                }
                // We just found the first UDP datagram with a data_provision message, now use this destination address all the time.
                _actual_dest = dst;
                verbose(u"using UDP destination address %s", dst);
            }

            // Extract TS packets from the data_provision message.
            ret_size = extractDataProvision(buffer, buffer_size, udp_data, udp_size);
            if (ret_size == 0) {
                continue; // no TS packets in this message
            }
        }
        else {
            // Look for raw TS.
            if (!_actual_dest.hasAddress() || !_actual_dest.hasPort()) {
                // The actual destination is not fully known yet.
                // We are still waiting for the first UDP datagram containing TS packets.
                // Is there any TS packet in this one?
                // This is just a check for presence, we don't use the returned packet position, count or size.
                size_t start_index = 0;
                size_t packet_count = 0;
                size_t packet_size = 0;
                if (!TSPacket::Locate(ip.protocolData(), ip.protocolDataSize(), start_index, packet_count, packet_size)) {
                    continue; // no TS packet in this UDP datagram.
                }
                // We just found the first UDP datagram with TS packets, now use this destination address all the time.
                _actual_dest = dst;
                verbose(u"using UDP destination address %s", dst);
            }

            // Now we have a valid UDP packet.
            ret_size = std::min(ip.protocolDataSize(), buffer_size);
            MemCopy(buffer, ip.protocolData(), ret_size);
        }

        // List all source addresses as they appear.
        if (_all_sources.find(src) == _all_sources.end()) {
            // This is a new source address.
            verbose(u"%s UDP source address %s", _all_sources.empty() ? u"using" : u"adding", src);
            _all_sources.insert(src);
        }

        // Adjust time stamps according to first one.
        if (timestamp >= cn::microseconds::zero()) {
            if (_first_tstamp < cn::microseconds::zero()) {
                // This is the first time stamp, the origin.
                _first_tstamp = timestamp;
                timestamp = cn::microseconds::zero();
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
// EMMG/PDG <=> MUX protocol TCP input method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::receiveEMMG(uint8_t *buffer, size_t buffer_size, size_t &ret_size, cn::microseconds& timestamp)
{
    // Read all TCP sessions matching the source and destination until eof or read TS packets.
    ret_size = 0;
    do {
        IPSocketAddress source;
        ByteBlock data;

        // Read a message header from any source: version(1), type(2), length(2).
        size_t size = 5;
        if (!_pcap_tcp.readTCP(source, data, size, timestamp, *this) || size < 5) {
            return false;
        }
        assert(data.size() == 5);

        // Read the rest of the message from the same source.
        size = GetUInt16(data.data() + 3);
        if (!_pcap_tcp.readTCP(source, data, size, timestamp, *this)) {
            return false;
        }

        // Try to extract TS packets from a data_provision message.
        ret_size = extractDataProvision(buffer, buffer_size, data.data(), data.size());

    } while (ret_size == 0);
    return true;
}


//----------------------------------------------------------------------------
// Check if a data area is an EMMG/PDG <=> MUX data_provision message.
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
// Extract TS packets from an EMMG/PDG <=> MUX data_provision message.
//----------------------------------------------------------------------------

size_t ts::PcapInputPlugin::extractDataProvision(uint8_t* buffer, size_t buffer_size, const uint8_t* msg, size_t msg_size)
{
    // If cannot be a data_provision message, no need to continue.
    if (!isDataProvision(msg, msg_size)) {
        return 0;
    }

    // Adjust protocol version when necessary.
    const ts::tlv::VERSION version = msg[0];
    if (version != _emmgmux.version()) {
        debug(u"switching EMMG <=> MUX version protocol to %d", version);
        _emmgmux.setVersion(version);
    }

    // Interpret the data as data_provision TLV message.
    tlv::MessagePtr ptr;
    tlv::MessageFactory mf(msg, msg_size, _emmgmux);
    if (mf.errorStatus() != tlv::OK) {
        return 0;
    }
    mf.factory(ptr);
    emmgmux::DataProvision* dprov = dynamic_cast<emmgmux::DataProvision*>(ptr.get());
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
        if (data != nullptr && !data->empty()) {
            if ((*data)[0] != SYNC_BYTE || data->size() % PKT_SIZE != 0) {
                warning(u"EMMG<=>MUX data_provision not likely TS packets, maybe in section mode");
                return 0;
            }
            const size_t dsize = std::min(buffer_size - ret_size, data->size());
            MemCopy(buffer + ret_size, data->data(), dsize);
            ret_size += dsize;
        }
    }
    return ret_size;
}


//----------------------------------------------------------------------------
// HTTP input method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::receiveHTTP(uint8_t *buffer, size_t buffer_size, size_t &ret_size, cn::microseconds& timestamp)
{
    ret_size = 0;

    // The first time, detect start of HTTP session or resynchronize on a TS packet.
    if (tsp->pluginPackets() == 0) {
        if (_pcap_tcp.startOfStream(*this)) {
            // At start of TCP session. At least one packet for this TCP session is ready to be read.
            // If the source was initially unspecified, it is now known.
            _actual_source = _pcap_tcp.sourceFilter();
            debug(u"at start of HTTP session, source: %s, server: %s", _actual_source, _pcap_tcp.serverPeer());
        }
        else {
            // The pcap file probably started in the middle of a TCP session.
            // Initially, the source may be unknown (if only the destination was specified).
            IPSocketAddress src(_source);
            size_t size = _http_chunk_size;
            if (_pcap_tcp.readTCP(src, _data, size, timestamp, *this)) {
                // The source is now known
                _actual_source = _pcap_tcp.sourceFilter();
                if (src != _actual_source) {
                    // The source was unknown and the data were read from the other direction.
                    // Revert the data buffer and retry to read from the real source.
                    _data.clear();
                    size = _http_chunk_size;
                    _pcap_tcp.readTCP(_actual_source, _data, size, timestamp, *this);
                }
            }
            if (size == 0) {
                // No initial packet in this TCP session, the session does not exist.
                verbose(u"TCP session not found in the pcap file");
                return false;
            }
            debug(u"start in middle of HTTP session, initial read: %'d bytes, source: %s", size, _actual_source);

            // Try to find 2 adjacent starts of packets (0x47).
            size_t start = 0;
            for (;;) {
                start = _data.find(SYNC_BYTE, start);
                if (start == NPOS || start + PKT_SIZE >= _data.size()) {
                    // Could not find two adjacent TS packets.
                    contentErrorHTTP();
                    return false;
                }
                else if (_data[start + PKT_SIZE] != SYNC_BYTE) {
                    // Found a sync byte but not sync with next packet, try further on.
                    start++;
                }
                else {
                    // Found two contiguous TS packets.
                    break;
                }
            }
            // Strip initial partial packet.
            _data_next = start;
        }
    }
    else if (_data_error) {
        // TCP stream alread marked as invalid.
        return false;
    }

    // Read and copy TS packets in the caller's buffer.
    while (buffer_size >= PKT_SIZE) {

        // Make sure we have enough data for two TS packets or a header line.
        if (_data_next + 1024 > _data.size()) {
            // Read more but don't fail on error, need to process what we already have in _data.
            size_t size = _http_chunk_size;
            const bool ok = _pcap_tcp.readTCP(_actual_source, _data, size, timestamp, *this);
            if (!ok) {
                debug(u"readTCP failed, read size: %'d bytes, position in file: %'d", size, _pcap_tcp.fileSize());
            }
        }

        // If less than a packet could be read in the buffer, this is the end of file.
        if (_data_next + PKT_SIZE > _data.size()) {
            break;
        }

        // In RTSP sessions, the packets may be encapsulated with a 4-byte header:
        // '$' one-byte-channel-id two-byte-length
        if (_data_next + 4 + PKT_SIZE <= _data.size() && _data[_data_next] == '$' && _data[_data_next + 4] == SYNC_BYTE) {
            // Ignore the RTSP data header.
            _data_next += 4;
        }

        if (_data_next + PKT_SIZE <= _data.size() && _data[_data_next] == SYNC_BYTE) {
            // Found one TS packet.
            MemCopy(buffer, _data.data() + _data_next, PKT_SIZE);
            _data_next += PKT_SIZE;
            buffer += PKT_SIZE;
            buffer_size -= PKT_SIZE;
            ret_size += PKT_SIZE;
        }
        else {
            // Must be an HTTP or RTSP header, an ASCII string, terminated by CR/LF.
            const size_t eol = _data.find('\n', _data_next);
            if (eol == NPOS) {
                // No header found, invalid content.
                contentErrorHTTP();
                break;
            }

            // Extract and skip the header line.
            std::string header(reinterpret_cast<const char*>(_data.data() + _data_next), eol - _data_next);
            _data_next = eol + 1;

            // Remove trailing spaces (cr/lf)
            while (!header.empty() && std::isspace(header.back())) {
                header.pop_back();
            }

            // Validate that the header contains only ASCII characters.
            // Otherwise, this is probably garbage binary data with a '\n' somewhere.
            for (auto c : header) {
                if (c < 0x20 || c > 0x7E) {
                    contentErrorHTTP();
                    header.clear();
                    break;
                }
            }

            // Display header in debug mode.
            if (!header.empty()) {
                debug(u"response header: %s", header);
            }
        }
    }

    // Cleanup internal buffer if it becomes too large.
    if (_data_next >= _data.size()) {
        _data.clear();
        _data_next = 0;
    }
    else if (_data.size() > 100 * PKT_SIZE) {
        _data.erase(0, _data_next);
        _data_next = 0;
    }
    return ret_size > 0;
}


//----------------------------------------------------------------------------
// Report an HTTP content error, return false.
//----------------------------------------------------------------------------

void ts::PcapInputPlugin::contentErrorHTTP()
{
    _data_error = true;
    _data.clear();
    _data_next = 0;
    error(u"content error, neither HTTP reponse headers nor TS packets in TCP stream");
}
