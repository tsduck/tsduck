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
//  Analysis tool for pcap and pcap-ng files.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsPcapStream.h"
#include "tsIPv4Packet.h"
#include "tsTime.h"
#include "tsBitRate.h"
#include "tsEMMGMUX.h"
#include "tsECMGSCS.h"
#include "tsPagerArgs.h"
#include "tstlvMessageFactory.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
// Command line options
//----------------------------------------------------------------------------

namespace {
    class Options: public ts::Args
    {
        TS_NOBUILD_NOCOPY(Options);
    public:
        Options(int argc, char *argv[]);

        ts::DuckContext       duck;     // TSDuck execution context.
        ts::PagerArgs         pager;    // Output paging options.
        ts::UString           input_file;
        bool                  print_summary;
        bool                  list_streams;
        bool                  print_intervals;
        bool                  dvb_simulcrypt;
        bool                  extract_tcp;
        std::set<uint8_t>     protocols;
        ts::IPv4SocketAddress source_filter;
        ts::IPv4SocketAddress dest_filter;
        ts::MicroSecond       interval;
    };
}

// Get command line options.
Options::Options(int argc, char *argv[]) :
    ts::Args(u"Analyze pcap and pcap-ng files", u"[options] [input-file]"),
    duck(this),
    pager(true, true),
    input_file(),
    print_summary(false),
    list_streams(false),
    print_intervals(false),
    dvb_simulcrypt(false),
    extract_tcp(false),
    protocols(),
    source_filter(),
    dest_filter(),
    interval(-1)
{
    ts::PcapFilter file;
    file.defineArgs(*this);
    pager.defineArgs(*this);

    option(u"", 0, FILENAME, 0, 1);
    help(u"", u"file-name",
         u"Input file in pcap or pcap-ng format, typically as saved by Wireshark. "
         u"Use the standard input if no file name is specified.");

    option(u"dvb-simulcrypt");
    help(u"dvb-simulcrypt",
         u"Dump the content of a session as DVB SimulCrypt protocol.\n"
         u"Without --udp, the first TCP session matching the --source and --destination options "
         u"is selected. The content of the session is interpreted as one of the TLV-based "
         u"DVB SimulCrypt protocols and all messages are formatted.\n"
         u"With --udp, all packets matching the --source and --destination options "
         u"are interpreted as EMMG/PDG <=> MUX protocol (this is the only DVB SimulCrypt "
         u"protocol which is based on UDP).");

    option(u"destination", 'd', STRING);
    help(u"destination", u"[address][:port]",
         u"Filter IPv4 packets based on the specified destination socket address. "
         u"The optional port number is used for TCP and UDP packets only.");

    option(u"extract-tcp-stream", 'e');
    help(u"extract-tcp-stream",
         u"Extract the content of a TCP session as hexadecimal dump. "
         u"The first TCP session matching the --source and --destination options is selected.");

    option(u"interval", 'i', POSITIVE);
    help(u"interval", u"micro-seconds",
         u"Print a summary of exchanged data by intervals of times in micro-seconds.");

    option(u"list-streams", 'l');
    help(u"list-streams",
         u"List all data streams. "
         u"A data streams is made of all packets from one source to one destination using one protocol.");

    option(u"source", 's', STRING);
    help(u"source", u"[address][:port]",
         u"Filter IPv4 packets based on the specified source socket address. "
         u"The optional port number is used for TCP and UDP packets only.");

    option(u"others", 'o');
    help(u"others", u"Filter packets from \"other\" protocols, i.e. neither TCP nor UDP.");

    option(u"tcp", 't');
    help(u"tcp", u"Filter TCP packets.");

    option(u"udp", 'u');
    help(u"udp", u"Filter UDP packets.");

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    pager.loadArgs(duck, *this);
    getValue(input_file, u"");
    const ts::UString dest_string(value(u"destination"));
    const ts::UString source_string(value(u"source"));
    getIntValue(interval, u"interval", 0);
    list_streams = present(u"list-streams");
    print_intervals = present(u"interval");
    dvb_simulcrypt = present(u"dvb-simulcrypt");
    extract_tcp = present(u"extract-tcp-stream");

    // Default is to print a summary of the file content.
    print_summary = !list_streams && !print_intervals;

    // Default is to filter all protocols (empty protocol set).
    if (present(u"tcp")) {
        protocols.insert(ts::IPv4_PROTO_TCP);
    }
    if (present(u"udp")) {
        protocols.insert(ts::IPv4_PROTO_UDP);
    }
    if (present(u"others")) {
        for (int p = 0; p < 256; ++p) {
            if (p != ts::IPv4_PROTO_TCP && p != ts::IPv4_PROTO_UDP) {
                protocols.insert(uint8_t(p));
            }
        }
    }

    // Decode network addresses.
    if (!source_string.empty()) {
        source_filter.resolve(source_string, *this);
    }
    if (!dest_string.empty()) {
        dest_filter.resolve(dest_string, *this);
    }

    // Final checking.
    if (dvb_simulcrypt && extract_tcp) {
        error(u"--dvb-simulcrypt and --extract-tcp-stream are mutually exclusive");
    }
    exitOnError();
}


//----------------------------------------------------------------------------
// Statistics data for a set of IP packets.
//----------------------------------------------------------------------------

namespace {
    class StatBlock
    {
    public:
        // Constructor.
        StatBlock();

        // Add statistics from one packet.
        void addPacket(const ts::IPv4Packet&, ts::MicroSecond);

        // Reset content, optionally set timestamps.
        void reset(ts::MicroSecond = -1);

        size_t          packet_count;      // number of IP packets in the data set
        size_t          total_ip_size;     // total size in bytes of IP packets, headers included
        size_t          total_data_size;   // total data size in bytes (TCP o UDP payload)
        ts::MicroSecond first_timestamp;   // negative if none found
        ts::MicroSecond last_timestamp;    // negative if none found
    };
}

// Constructor.
StatBlock::StatBlock() :
    packet_count(0),
    total_ip_size(0),
    total_data_size(0),
    first_timestamp(-1),
    last_timestamp(-1)
{
}

// Reset content, optionally set timestamps.
void StatBlock::reset(ts::MicroSecond timestamps)
{
    packet_count = total_ip_size = total_data_size = 0;
    first_timestamp = last_timestamp = timestamps;
}

// Add statistics from one packet.
void StatBlock::addPacket(const ts::IPv4Packet& ip, ts::MicroSecond timestamp)
{
    packet_count++;
    total_ip_size += ip.size();
    total_data_size += ip.protocolDataSize();
    if (timestamp >= 0) {
        if (first_timestamp < 0) {
            first_timestamp = timestamp;
        }
        last_timestamp = timestamp;
    }
}


//----------------------------------------------------------------------------
// Identification of one "data stream".
//----------------------------------------------------------------------------

namespace {
    class StreamId
    {
    public:
        ts::IPv4SocketAddress source;
        ts::IPv4SocketAddress destination;
        uint8_t               protocol;

        // Constructor.
        StreamId(const ts::IPv4SocketAddress& src = ts::IPv4SocketAddress(), const ts::IPv4SocketAddress& dst = ts::IPv4SocketAddress(), uint8_t proto = 0xFF);

        // Comparison, for use in containers.
        bool operator<(const StreamId& other) const;
    };
}

StreamId::StreamId(const ts::IPv4SocketAddress& src, const ts::IPv4SocketAddress& dst, uint8_t proto) :
    source(src),
    destination(dst),
    protocol(proto)
{
}

bool StreamId::operator<(const StreamId& other) const
{
    if (source != other.source) {
        return source < other.source;
    }
    else if (destination != other.destination) {
        return destination < other.destination;
    }
    else {
        return protocol < other.protocol;
    }
}


//----------------------------------------------------------------------------
// Display summary of content by intervals of time.
//----------------------------------------------------------------------------

namespace {
    class DisplayInterval
    {
        TS_NOBUILD_NOCOPY(DisplayInterval);
    public:
        // Constructor.
        DisplayInterval(Options&);

        // Process one IPv4 packet.
        void addPacket(std::ostream&, const ts::PcapFile&, const ts::IPv4Packet&, ts::MicroSecond);

        // Terminate output.
        void close(std::ostream&, const ts::PcapFile&);

    private:
        Options&  _opt;
        StatBlock _stats;

        // Print current line and reset stats.
        void print(std::ostream&, const ts::PcapFile&);
    };
}

// Constructor.
DisplayInterval::DisplayInterval(Options& opt) :
    _opt(opt),
    _stats()
{
}

// Print current line and reset stats.
void DisplayInterval::print(std::ostream& out, const ts::PcapFile& file)
{
    out << ts::UString::Format(u"%-24s %+16'd %11'd %15'd %12'd",
                               {ts::PcapFile::ToTime(_stats.first_timestamp),
                                file.timeOffset(_stats.first_timestamp),
                                _stats.packet_count,
                                _stats.total_data_size,
                                ts::BitRate(_stats.total_data_size * 8 * ts::MicroSecPerSec) / _opt.interval})
        << std::endl;
    _stats.reset(_stats.first_timestamp + _opt.interval);
}

// Process one IPv4 packet.
void DisplayInterval::addPacket(std::ostream& out, const ts::PcapFile& file, const ts::IPv4Packet& ip, ts::MicroSecond timestamp)
{
    // Without timestamp, we cannot do anything.
    if (timestamp >= 0) {
        if (_stats.first_timestamp < 0) {
            // Initial processing.
            out << std::endl;
            out << ts::UString::Format(u"%-24s %16s %11s %15s %12s", {u"Date", u"Micro-seconds", u"Packets", u"Data bytes", u"Bitrate"})
                << std::endl;
        }
        else {
            // Print all previous intervals.
            while (timestamp > _stats.first_timestamp + _opt.interval) {
                print(out, file);
            }
        }
        _stats.addPacket(ip, timestamp);
    }
}

// Terminate output.
void DisplayInterval::close(std::ostream& out, const ts::PcapFile& file)
{
    if (_stats.packet_count > 0) {
        print(out, file);
    }
    out << std::endl;
}


//----------------------------------------------------------------------------
// Display file analysis.
//----------------------------------------------------------------------------

namespace {
    class FileAnalysis
    {
        TS_NOBUILD_NOCOPY(FileAnalysis);
    public:
        // Constructor.
        FileAnalysis(Options&);

        // Analyse the file, return true on success, false on error.
        bool analyze(std::ostream&);

    private:
        Options&        _opt;
        ts::PcapFilter  _file;
        DisplayInterval _interval;                   // Display stats by time intervals.
        StatBlock       _global_stats;               // Global stats
        std::map<StreamId,StatBlock> _streams_stats; // Stats per data stream.

        // Display summary of content.
        void displaySummary(std::ostream& out, const StatBlock& stats);

        // Display list of streams.
        void listStreams(std::ostream& out, ts::MicroSecond duration);
    };
}

// Constructor.
FileAnalysis::FileAnalysis(Options& opt) :
    _opt(opt),
    _file(),
    _interval(opt),
    _global_stats(),
    _streams_stats()
{
}

// Analyse the file, return true on success, false on error.
bool FileAnalysis::analyze(std::ostream& out)
{
    // Open the pcap file.
    if (!_file.loadArgs(_opt.duck, _opt) || !_file.open(_opt.input_file, _opt)) {
        return false;
    }

    // Set packet filters.
    _file.setProtocolFilter(_opt.protocols);
    _file.setSourceFilter(_opt.source_filter);
    _file.setDestinationFilter(_opt.dest_filter);

    // Read all IPv4 packets from the file.
    ts::IPv4Packet ip;
    ts::MicroSecond timestamp = 0;
    while (_file.readIPv4(ip, timestamp, _opt)) {
        _global_stats.addPacket(ip, timestamp);
        if (_opt.list_streams) {
            _streams_stats[StreamId(ip.sourceSocketAddress(), ip.destinationSocketAddress(), ip.protocol())].addPacket(ip, timestamp);
        }
        if (_opt.print_intervals) {
            _interval.addPacket(out, _file, ip, timestamp);
        }
    }
    _file.close();

    // Print final data.
    if (_opt.print_intervals) {
        _interval.close(out, _file);
    }
    if (_opt.list_streams) {
        listStreams(out, _global_stats.last_timestamp - _global_stats.first_timestamp);
    }
    if (_opt.print_summary) {
        displaySummary(out, _global_stats);
    }
    return true;
}

// Display summary of content.
void FileAnalysis::displaySummary(std::ostream& out, const StatBlock& stats)
{
    const size_t hwidth = 22; // header width

    out << std::endl;
    out << "File summary:" << std::endl;
    out << ts::UString::Format(u"  %-*s %'d", {hwidth, u"Total packets in file:", _file.packetCount()}) << std::endl;
    out << ts::UString::Format(u"  %-*s %'d", {hwidth, u"Total IPv4 packets:", _file.ipv4PacketCount()}) << std::endl;
    out << ts::UString::Format(u"  %-*s %'d bytes", {hwidth, u"File size:", _file.fileSize()}) << std::endl;
    out << ts::UString::Format(u"  %-*s %'d bytes", {hwidth, u"Total packets size:", _file.totalPacketsSize()}) << std::endl;
    out << ts::UString::Format(u"  %-*s %'d bytes", {hwidth, u"Total IPv4 size:", _file.totalIPv4PacketsSize()}) << std::endl;
    out << std::endl;

    out << "Filtered packets summary:" << std::endl;
    out << ts::UString::Format(u"  %-*s %'d", {hwidth, u"Packets:", stats.packet_count}) << std::endl;
    out << ts::UString::Format(u"  %-*s %'d", {hwidth, u"Packets size:", stats.total_ip_size}) << std::endl;
    out << ts::UString::Format(u"  %-*s %'d", {hwidth, u"Payload data size:", stats.total_data_size}) << std::endl;

    if (stats.first_timestamp > 0 && stats.last_timestamp > 0) {
        const ts::Time start(ts::PcapFile::ToTime(stats.first_timestamp));
        const ts::Time end(ts::PcapFile::ToTime(stats.last_timestamp));
        const ts::MicroSecond duration = stats.last_timestamp - stats.first_timestamp;
        out << ts::UString::Format(u"  %-*s %s (%+'d micro-seconds)", {hwidth, u"Start time:", start, _file.timeOffset(stats.first_timestamp)}) << std::endl;
        out << ts::UString::Format(u"  %-*s %s (%+'d micro-seconds)", {hwidth, u"End time:", end, _file.timeOffset(stats.last_timestamp)}) << std::endl;
        if (duration > 0) {
            out << ts::UString::Format(u"  %-*s %'d micro-seconds", {hwidth, u"Duration:", duration}) << std::endl;
            out << ts::UString::Format(u"  %-*s %'d bits/second", {hwidth, u"IP bitrate:", (ts::BitRate(stats.total_ip_size * 8 * ts::MicroSecPerSec) / duration)}) << std::endl;
            out << ts::UString::Format(u"  %-*s %'d bits/second", {hwidth, u"Data bitrate:", (ts::BitRate(stats.total_data_size * 8 * ts::MicroSecPerSec) / duration)}) << std::endl;
        }
    }
    out << std::endl;
}

// Display list of streams.
void FileAnalysis::listStreams(std::ostream& out, ts::MicroSecond duration)
{
    out << std::endl
        << ts::UString::Format(u"%-22s %-22s %-8s %11s %15s %12s", {u"Source", u"Destination", u"Protocol", u"Packets", u"Data bytes", u"Bitrate"})
        << std::endl;
    for (const auto& it : _streams_stats) {
        const StreamId& id(it.first);
        const StatBlock& sb(it.second);
        out << ts::UString::Format(u"%-22s %-22s %-8s %11'd %15'd %12'd",
                                   {id.source,
                                    id.destination,
                                    ts::IPProtocolName(id.protocol),
                                    sb.packet_count,
                                    sb.total_data_size,
                                    duration <= 0 ? 0 : (ts::BitRate(sb.total_data_size * 8 * ts::MicroSecPerSec) / duration)})
            << std::endl;
    }
    out << std::endl;
}


//----------------------------------------------------------------------------
// DVB SimulCrypt dump, base class.
//----------------------------------------------------------------------------

namespace {
    class SimulCryptDump
    {
        TS_NOBUILD_NOCOPY(SimulCryptDump);
    protected:
        // Constructor.
        SimulCryptDump(Options&);

        // Dump a message.
        void dumpMessage(std::ostream&, const uint8_t*, size_t, const ts::IPv4SocketAddress& src, const ts::IPv4SocketAddress& dst, ts::MicroSecond timestamp);

        // Protected fields
        Options& _opt;
    };
}

// Constructor.
SimulCryptDump::SimulCryptDump(Options& opt) :
    _opt(opt)
{
}

// Dump a message.
void SimulCryptDump::dumpMessage(std::ostream& out, const uint8_t* data, size_t size, const ts::IPv4SocketAddress& src, const ts::IPv4SocketAddress& dst, ts::MicroSecond timestamp)
{
    // Build a message description.
    if (timestamp > 0) {
        out << (ts::Time::UnixEpoch + timestamp / ts::MicroSecPerMilliSec) << ", ";
    }
    out << src << " -> " << dst << ", " << size << " bytes" << std::endl;

    // There must be 5 header bytes: version(1), type(2), length(2).
    // See ETSI TS 103 197, section 4.4.1.
    bool valid = size >= 5;
    const uint16_t msg_type = valid ? ts::GetUInt16(data + 1) : 0;
    const size_t msg_size = valid ? 5 + ts::GetUInt16(data + 3) : 0;
    valid = valid && size >= msg_size;

    // Determine the DVB SimulCrypt protocol. We currently only support ECMG and EMMG.
    ts::tlv::Protocol* protocol = nullptr;
    if (valid) {
        if (ts::ecmgscs::IsValidCommand(msg_type)) {
            protocol = ts::ecmgscs::Protocol::Instance();
        }
        else if (ts::emmgmux::IsValidCommand(msg_type)) {
            protocol = ts::emmgmux::Protocol::Instance();
        }
        else {
            valid = false;
        }
    }

    // Decode the message.
    if (valid) {
        // Adjust protocol version when necessary.
        const ts::tlv::VERSION version = data[0];
        if (version != protocol->version()) {
            _opt.debug(u"switching EMMG <=> MUX version protocol to %d", {version});
            protocol->setVersion(version);
        }

        // Interpret the UDP message as TLV message.
        ts::tlv::MessagePtr msg;
        ts::tlv::MessageFactory mf(data, msg_size, protocol);
        valid = false;
        if (mf.errorStatus() == ts::tlv::OK) {
            mf.factory(msg);
            if (!msg.isNull()) {
                valid = true;
                out << msg->dump(4);
            }
        }
    }

    // Display invalid messages.
    if (!valid) {
        out << "    Invalid or unsupported DVB SimulCrypt message" << std::endl;
        out << ts::UString::Dump(data, size, ts::UString::ASCII | ts::UString::HEXA | ts::UString::OFFSET | ts::UString::BPL, 4, 16);
    }
    else if (size > msg_size) {
        out << "    " << (size - msg_size) << " extraneous bytes:" << std::endl;
        out << ts::UString::Dump(data + msg_size, size - msg_size, ts::UString::ASCII | ts::UString::HEXA | ts::UString::OFFSET | ts::UString::BPL, 4, 16);
    }
    out << std::endl;
}


//----------------------------------------------------------------------------
// DVB SimulCrypt dump, UDP mode.
//----------------------------------------------------------------------------

namespace {
    class UDPSimulCryptDump: private SimulCryptDump
    {
        TS_NOBUILD_NOCOPY(UDPSimulCryptDump);
    public:
        // Constructor.
        UDPSimulCryptDump(Options&);

        // Dump the file, return true on success, false on error.
        bool dump(std::ostream&);

    private:
        ts::PcapFilter _file;
    };
}

// Constructor.
UDPSimulCryptDump::UDPSimulCryptDump(Options& opt) :
    SimulCryptDump(opt),
    _file()
{
}

// Dump the file, return true on success, false on error.
bool UDPSimulCryptDump::dump(std::ostream& out)
{
    // Open the pcap file.
    if (!_file.loadArgs(_opt.duck, _opt) || !_file.open(_opt.input_file, _opt)) {
        return false;
    }

    // Set packet filters.
    _file.setProtocolFilterUDP();
    _file.setSourceFilter(_opt.source_filter);
    _file.setDestinationFilter(_opt.dest_filter);

    // Read all UDP packets matching the source and destination.
    ts::IPv4Packet ip;
    ts::MicroSecond timestamp = 0;
    while (_file.readIPv4(ip, timestamp, _opt)) {
        // Dump the content of the UDP datagram as DVB SimulCrypt message.
        dumpMessage(out, ip.protocolData(), ip.protocolDataSize(), ip.sourceAddress(), ip.destinationAddress(), timestamp);
    }
    _file.close();
    return true;
}


//----------------------------------------------------------------------------
// DVB SimulCrypt dump, TCP mode.
//----------------------------------------------------------------------------

namespace {
    class TCPSimulCryptDump: private SimulCryptDump
    {
        TS_NOBUILD_NOCOPY(TCPSimulCryptDump);
    public:
        // Constructor.
        TCPSimulCryptDump(Options&);

        // Dump the file, return true on success, false on error.
        bool dump(std::ostream&);

    private:
        ts::PcapStream _file;
    };
}

// Constructor.
TCPSimulCryptDump::TCPSimulCryptDump(Options& opt) :
    SimulCryptDump(opt),
    _file()
{
}

// Dump the file, return true on success, false on error.
bool TCPSimulCryptDump::dump(std::ostream& out)
{
    // Open the pcap file.
    if (!_file.loadArgs(_opt.duck, _opt) || !_file.open(_opt.input_file, _opt)) {
        return false;
    }

    // Set packet filters.
    _file.setBidirectionalFilter(_opt.source_filter, _opt.dest_filter);

    // Read all TCP sessions matching the source and destination.
    for (;;) {
        ts::MicroSecond timestamp = 0;
        ts::IPv4SocketAddress source;
        ts::ByteBlock data;

        // Read a message header from any source.
        // There must be 5 header bytes: version(1), type(2), length(2).
        // See ETSI TS 103 197, section 4.4.1.
        size_t size = 5;
        if (!_file.readTCP(source, data, size, timestamp, _opt)) {
            break;
        }
        if (size < 5) {
            _opt.error(u"truncated message: %s (%s -> %s)", {ts::UString::Dump(data, ts::UString::SINGLE_LINE), source, _file.otherFilter(source)});
            break;
        }
        assert(data.size() == 5);
        assert(source.hasAddress());
        assert(source.hasPort());

        // Read the rest of the message from the same source.
        size = ts::GetUInt16(data.data() + 3);
        if (!_file.readTCP(source, data, size, timestamp, _opt)) {
            break;
        }

        // Dump the content of the message as DVB SimulCrypt.
        dumpMessage(out, data.data(), data.size(), source, _file.otherFilter(source), timestamp);
    }
    _file.close();
    return true;
}


//----------------------------------------------------------------------------
// Extract TCP session
//----------------------------------------------------------------------------

namespace {
    class TCPSessionDump
    {
        TS_NOBUILD_NOCOPY(TCPSessionDump);
    public:
        // Constructor.
        TCPSessionDump(Options&);

        // Dump the session, return true on success, false on error.
        bool dump(std::ostream&);

    private:
        Options&       _opt;
        ts::PcapStream _file;

        // Dump a message.
        void dumpMessage(std::ostream&, const ts::ByteBlock&, const ts::IPv4SocketAddress& src, const ts::IPv4SocketAddress& dst, ts::MicroSecond timestamp);
    };
}

// Constructor.
TCPSessionDump::TCPSessionDump(Options& opt) :
    _opt(opt),
    _file()
{
}

// Dump a message.
void TCPSessionDump::dumpMessage(std::ostream& out, const ts::ByteBlock& data, const ts::IPv4SocketAddress& src, const ts::IPv4SocketAddress& dst, ts::MicroSecond timestamp)
{
    if (!data.empty()) {
        if (timestamp > 0) {
            out << (ts::Time::UnixEpoch + timestamp / ts::MicroSecPerMilliSec) << ", ";
        }
        out << src << " -> " << dst << ", " << data.size() << " bytes" << std::endl;
        out << ts::UString::Dump(data, ts::UString::ASCII | ts::UString::HEXA | ts::UString::OFFSET | ts::UString::BPL, 4, 16);
        out << std::endl;
    }
}

// Dump the session, return true on success, false on error.
bool TCPSessionDump::dump(std::ostream& out)
{
    // Open the pcap file.
    if (!_file.loadArgs(_opt.duck, _opt) || !_file.open(_opt.input_file, _opt)) {
        return false;
    }

    // Set packet filters.
    _file.setBidirectionalFilter(_opt.source_filter, _opt.dest_filter);

    ts::ByteBlock data;
    ts::MicroSecond data_timestamp = 0;
    ts::IPv4SocketAddress data_source;
    ts::IPv4SocketAddress data_dest;
    ts::ByteBlock buf;
    ts::IPv4SocketAddress buf_source;

    // Read all TCP sessions matching the source and destination.
    for (;;) {
        // Read byte by byte, to make sure the alternance between client and server traffic is clearly identified.
        buf.clear();
        buf_source.clear();
        size_t size = 1;
        ts::MicroSecond timestamp = 0;
        if (!_file.readTCP(buf_source, buf, size, timestamp, _opt)) {
            break;
        }
        if (data_timestamp <= 0) {
            data_timestamp = timestamp;
        }

        if (!buf_source.match(data_source)) {
            // New direction, dump previous message.
            dumpMessage(out, data, data_source, data_dest, data_timestamp);
            data.clear();
            data_timestamp = timestamp;
        }

        data_source = buf_source;
        data_dest = _file.otherFilter(buf_source);
        data.append(buf);
    }

    // Dump remaining data, if any.
    dumpMessage(out, data, data_source, data_dest, data_timestamp);
    _file.close();
    return true;
}


//----------------------------------------------------------------------------
// Program main code.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Get command line options.
    Options opt(argc, argv);
    bool status = true;

    // Output device, may be paginated.
    std::ostream& out(opt.pager.output(opt));

    if (opt.extract_tcp) {
        // TCP session dump.
        TCPSessionDump tcp(opt);
        status = tcp.dump(out);
    }
    else if (!opt.dvb_simulcrypt) {
        // Global file analysis by default.
        FileAnalysis dfa(opt);
        status = dfa.analyze(out);
    }
    else if (opt.protocols.find(ts::IPv4_PROTO_UDP) == opt.protocols.end()) {
        // DVB SimulCrypt dump, TCP mode.
        TCPSimulCryptDump dvb(opt);
        status = dvb.dump(out);
    }
    else {
        // DVB SimulCrypt dump, UDP mode.
        UDPSimulCryptDump dvb(opt);
        status = dvb.dump(out);
    }

    return status ? EXIT_SUCCESS : EXIT_FAILURE;
}
