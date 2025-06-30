//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Analysis tool for pcap and pcap-ng files.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsPcapStream.h"
#include "tsIPPacket.h"
#include "tsTime.h"
#include "tsTS.h"
#include "tsBitRate.h"
#include "tsEMMGMUX.h"
#include "tsECMGSCS.h"
#include "tsPagerArgs.h"
#include "tsTextTable.h"
#include "tsSysUtils.h"
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

        ts::DuckContext       duck {this};
        ts::PagerArgs         pager {true, true};
        ts::UString           input_file {};
        ts::UString           output_file {};
        bool                  print_summary = false;
        bool                  list_streams = false;
        bool                  print_intervals = false;
        bool                  dvb_simulcrypt = false;
        bool                  extract_tcp = false;
        bool                  save_tcp = false;
        std::set<uint8_t>     protocols {};
        ts::IPSocketAddress   source_filter {};
        ts::IPSocketAddress   dest_filter {};
        cn::microseconds      interval = cn::microseconds(-1);
        ts::emmgmux::Protocol emmgmux {};
        ts::ecmgscs::Protocol ecmgscs {};
    };
}

// Get command line options.
Options::Options(int argc, char *argv[]) :
    ts::Args(u"Analyze pcap and pcap-ng files", u"[options] [input-file]")
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

    option(u"destination", 'd', IPSOCKADDR_OAP);
    help(u"destination",
         u"Filter IP packets based on the specified destination socket address. "
         u"The optional port number is used for TCP and UDP packets only.");

    option(u"extract-tcp-stream", 'e');
    help(u"extract-tcp-stream",
         u"Extract the content of a TCP session as hexadecimal dump. "
         u"The two directions of the TCP session are dumped. "
         u"The first TCP session matching the --source and --destination options is selected.");

    option<cn::microseconds>(u"interval", 'i');
    help(u"interval",
         u"Print a summary of exchanged data by intervals of times in micro-seconds.");

    option(u"list-streams", 'l');
    help(u"list-streams",
         u"List all data streams. "
         u"A data streams is made of all packets from one source to one destination using one protocol.");

    option(u"others");
    help(u"others", u"Filter packets from \"other\" protocols, i.e. neither TCP nor UDP.");

    option(u"output-tcp-stream", 'o', FILENAME);
    help(u"output-tcp-stream",
         u"Extract the content of a TCP session and save it in the specified binary file. "
         u"The first TCP session matching the --source and --destination options is selected. "
         u"Unlike --extract-tcp-stream, only one side of the TCP session is saved, from --source to --destination. "
         u"If the file name is \"-\", the standard output is used.");

    option(u"source", 's', IPSOCKADDR_OAP);
    help(u"source",
         u"Filter IP packets based on the specified source socket address. "
         u"The optional port number is used for TCP and UDP packets only.");

    option(u"tcp", 't');
    help(u"tcp", u"Filter TCP packets.");

    option(u"udp", 'u');
    help(u"udp", u"Filter UDP packets.");

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    pager.loadArgs(*this);
    getValue(input_file, u"");
    getValue(output_file, u"output-tcp-stream");
    save_tcp = present(u"output-tcp-stream");
    getSocketValue(dest_filter, u"destination");
    getSocketValue(source_filter, u"source");
    getChronoValue(interval, u"interval");
    list_streams = present(u"list-streams");
    print_intervals = present(u"interval");
    dvb_simulcrypt = present(u"dvb-simulcrypt");
    extract_tcp = present(u"extract-tcp-stream");

    // Default is to print a summary of the file content.
    print_summary = !list_streams && !print_intervals;

    // Default is to filter all protocols (empty protocol set).
    if (present(u"tcp")) {
        protocols.insert(ts::IP_SUBPROTO_TCP);
    }
    if (present(u"udp")) {
        protocols.insert(ts::IP_SUBPROTO_UDP);
    }
    if (present(u"others")) {
        for (int p = 0; p < 256; ++p) {
            if (p != ts::IP_SUBPROTO_TCP && p != ts::IP_SUBPROTO_UDP) {
                protocols.insert(uint8_t(p));
            }
        }
    }

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
        size_t           packet_count = 0;      // number of IP packets in the data set
        size_t           total_ip_size = 0;     // total size in bytes of IP packets, headers included
        size_t           total_data_size = 0;   // total data size in bytes (TCP o UDP payload)
        cn::microseconds first_timestamp = cn::microseconds(-1);  // negative if none found
        cn::microseconds last_timestamp = cn::microseconds(-1);   // negative if none found

        // Constructor.
        StatBlock() = default;

        // Add statistics from one packet.
        void addPacket(const ts::IPPacket&, cn::microseconds);

        // Reset content, optionally set timestamps.
        void reset(cn::microseconds = cn::microseconds(-1));
    };
}

// Reset content, optionally set timestamps.
void StatBlock::reset(cn::microseconds timestamps)
{
    packet_count = total_ip_size = total_data_size = 0;
    first_timestamp = last_timestamp = timestamps;
}

// Add statistics from one packet.
void StatBlock::addPacket(const ts::IPPacket& ip, cn::microseconds timestamp)
{
    packet_count++;
    total_ip_size += ip.size();
    total_data_size += ip.protocolDataSize();
    if (timestamp >= cn::microseconds::zero()) {
        if (first_timestamp < cn::microseconds::zero()) {
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
        ts::VLANIdStack     vlans;
        ts::IPSocketAddress source;
        ts::IPSocketAddress destination;
        uint8_t             protocol;

        // Comparison, for use in containers.
        bool operator<(const StreamId& other) const;
    };
}

bool StreamId::operator<(const StreamId& other) const
{
    if (vlans != other.vlans) {
        return vlans < other.vlans;
    }
    else if (source != other.source) {
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
        DisplayInterval(Options& opt) : _opt(opt) {}

        // Process one IP packet.
        void addPacket(std::ostream&, const ts::PcapFile&, const ts::IPPacket&, cn::microseconds);

        // Terminate output.
        void close(std::ostream&, const ts::PcapFile&);

    private:
        Options&  _opt;
        StatBlock _stats {};

        // Print current line and reset stats.
        void print(std::ostream&, const ts::PcapFile&);
    };
}

// Print current line and reset stats.
void DisplayInterval::print(std::ostream& out, const ts::PcapFile& file)
{
    out << ts::UString::Format(u"%-24s %+16'd %11'd %15'd %12'd",
                               ts::PcapFile::ToTime(_stats.first_timestamp),
                               file.timeOffset(_stats.first_timestamp).count(),
                               _stats.packet_count,
                               _stats.total_data_size,
                               ts::BytesBitRate(_stats.total_data_size, _opt.interval))
        << std::endl;
    _stats.reset(_stats.first_timestamp + _opt.interval);
}

// Process one IP packet.
void DisplayInterval::addPacket(std::ostream& out, const ts::PcapFile& file, const ts::IPPacket& ip, cn::microseconds timestamp)
{
    // Without timestamp, we cannot do anything.
    if (timestamp >= cn::microseconds::zero()) {
        if (_stats.first_timestamp < cn::microseconds::zero()) {
            // Initial processing.
            out << std::endl
                << ts::UString::Format(u"%-24s %16s %11s %15s %12s", u"Date", u"Micro-seconds", u"Packets", u"Data bytes", u"Bitrate")
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
        FileAnalysis(Options& opt) : _opt(opt), _interval(opt) {}

        // Analyse the file, return true on success, false on error.
        bool analyze(std::ostream&);

    private:
        Options&        _opt;
        ts::PcapFilter  _file {};
        DisplayInterval _interval;                      // Display stats by time intervals.
        StatBlock       _global_stats {};               // Global stats
        std::map<StreamId,StatBlock> _streams_stats {}; // Stats per data stream.

        // Display summary of content.
        void displaySummary(std::ostream& out, const StatBlock& stats);

        // Display list of streams.
        void listStreams(std::ostream& out, cn::microseconds duration);
    };
}

// Analyse the file, return true on success, false on error.
bool FileAnalysis::analyze(std::ostream& out)
{
    // Open the pcap file.
    if (!_file.loadArgs(_opt) || !_file.open(_opt.input_file, _opt)) {
        return false;
    }

    // Set packet filters.
    _file.setProtocolFilter(_opt.protocols);
    _file.setSourceFilter(_opt.source_filter);
    _file.setDestinationFilter(_opt.dest_filter);

    // Read all IP packets from the file.
    ts::IPPacket ip;
    ts::VLANIdStack vlans;
    cn::microseconds timestamp = cn::microseconds::zero();
    while (_file.readIP(ip, vlans, timestamp, _opt)) {
        _global_stats.addPacket(ip, timestamp);
        if (_opt.list_streams) {
            _streams_stats[{vlans, ip.source(), ip.destination(), ip.protocol()}].addPacket(ip, timestamp);
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
    out << ts::UString::Format(u"  %-*s %'d", hwidth, u"Total packets in file:", _file.packetCount()) << std::endl;
    out << ts::UString::Format(u"  %-*s %'d", hwidth, u"Total IP packets:", _file.ipPacketCount()) << std::endl;
    out << ts::UString::Format(u"  %-*s %'d bytes", hwidth, u"File size:", _file.fileSize()) << std::endl;
    out << ts::UString::Format(u"  %-*s %'d bytes", hwidth, u"Total packets size:", _file.totalPacketsSize()) << std::endl;
    out << ts::UString::Format(u"  %-*s %'d bytes", hwidth, u"Total IP size:", _file.totalIPPacketsSize()) << std::endl;
    out << std::endl;

    out << "Filtered packets summary:" << std::endl;
    out << ts::UString::Format(u"  %-*s %'d", hwidth, u"Packets:", stats.packet_count) << std::endl;
    out << ts::UString::Format(u"  %-*s %'d", hwidth, u"Packets size:", stats.total_ip_size) << std::endl;
    out << ts::UString::Format(u"  %-*s %'d", hwidth, u"Payload data size:", stats.total_data_size) << std::endl;

    if (stats.first_timestamp > cn::microseconds::zero() && stats.last_timestamp > cn::microseconds::zero()) {
        const ts::Time start(ts::PcapFile::ToTime(stats.first_timestamp));
        const ts::Time end(ts::PcapFile::ToTime(stats.last_timestamp));
        const cn::microseconds duration = stats.last_timestamp - stats.first_timestamp;
        out << ts::UString::Format(u"  %-*s %s (%+'s)", hwidth, u"Start time:", start, _file.timeOffset(stats.first_timestamp)) << std::endl;
        out << ts::UString::Format(u"  %-*s %s (%+'s)", hwidth, u"End time:", end, _file.timeOffset(stats.last_timestamp)) << std::endl;
        if (duration > cn::microseconds::zero()) {
            out << ts::UString::Format(u"  %-*s %'s", hwidth, u"Duration:", duration) << std::endl;
            out << ts::UString::Format(u"  %-*s %'d bits/second", hwidth, u"IP bitrate:", ts::BytesBitRate(stats.total_ip_size, duration)) << std::endl;
            out << ts::UString::Format(u"  %-*s %'d bits/second", hwidth, u"Data bitrate:", ts::BytesBitRate(stats.total_data_size, duration)) << std::endl;
        }
    }
    out << std::endl;
}

// Display list of streams.
void FileAnalysis::listStreams(std::ostream& out, cn::microseconds duration)
{
    using Align = ts::TextTable::Align;
    enum Id {VLAN, SRC, DEST, PROTO, PKTS, BYTES, BITRATE};

    ts::TextTable table;
    table.addColumn(VLAN, u"VLAN", Align::LEFT);
    table.addColumn(SRC, u"Source", Align::LEFT);
    table.addColumn(DEST, u"Destination", Align::LEFT);
    table.addColumn(PROTO, u"Protocol", Align::LEFT);
    table.addColumn(PKTS, u"Packets", Align::RIGHT);
    table.addColumn(BYTES, u"Data bytes", Align::RIGHT);
    table.addColumn(BITRATE, u"Bitrate", Align::RIGHT);

    for (const auto& it : _streams_stats) {
        const StreamId& id(it.first);
        const StatBlock& sb(it.second);
        table.newLine();
        table.setCell(VLAN, id.vlans.toString());
        table.setCell(SRC, id.source);
        table.setCell(DEST, id.destination);
        table.setCell(PROTO, ts::IPProtocolName(id.protocol));
        table.setCell(PKTS, ts::UString::Decimal(sb.packet_count));
        table.setCell(BYTES, ts::UString::Decimal(sb.total_data_size));
        table.setCell(BITRATE, ts::UString::Decimal(duration <= cn::microseconds::zero() ? 0 : ts::BytesBitRate(sb.total_data_size, duration).toInt()));
    }

    out << std::endl;
    table.output(out, ts::TextTable::Headers::TEXT, true, u"", u"  ");
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
        SimulCryptDump(Options& opt) : _opt(opt) {}

        // Dump a message.
        void dumpMessage(std::ostream&, const uint8_t*, size_t, const ts::IPSocketAddress& src, const ts::IPSocketAddress& dst, cn::microseconds timestamp);

        // Protected fields
        Options& _opt;
    };
}

// Dump a message.
void SimulCryptDump::dumpMessage(std::ostream& out, const uint8_t* data, size_t size, const ts::IPSocketAddress& src, const ts::IPSocketAddress& dst, cn::microseconds timestamp)
{
    // Build a message description.
    if (timestamp > cn::microseconds::zero()) {
        out << (ts::Time::UnixEpoch + timestamp) << ", ";
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
            protocol = &_opt.ecmgscs;
        }
        else if (ts::emmgmux::IsValidCommand(msg_type)) {
            protocol = &_opt.emmgmux;
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
            _opt.debug(u"switching EMMG <=> MUX version protocol to %d", version);
            protocol->setVersion(version);
        }

        // Interpret the UDP message as TLV message.
        ts::tlv::MessagePtr msg;
        ts::tlv::MessageFactory mf(data, msg_size, *protocol);
        valid = false;
        if (mf.errorStatus() == ts::tlv::OK) {
            mf.factory(msg);
            if (msg != nullptr) {
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
        UDPSimulCryptDump(Options& opt) : SimulCryptDump(opt) {}

        // Dump the file, return true on success, false on error.
        bool dump(std::ostream&);

    private:
        ts::PcapFilter _file {};
    };
}

// Dump the file, return true on success, false on error.
bool UDPSimulCryptDump::dump(std::ostream& out)
{
    // Open the pcap file.
    if (!_file.loadArgs(_opt) || !_file.open(_opt.input_file, _opt)) {
        return false;
    }

    // Set packet filters.
    _file.setProtocolFilterUDP();
    _file.setSourceFilter(_opt.source_filter);
    _file.setDestinationFilter(_opt.dest_filter);

    // Read all UDP packets matching the source and destination.
    ts::IPPacket ip;
    ts::VLANIdStack vlans;
    cn::microseconds timestamp = cn::microseconds::zero();
    while (_file.readIP(ip, vlans, timestamp, _opt)) {
        // Dump the content of the UDP datagram as DVB SimulCrypt message.
        dumpMessage(out, ip.protocolData(), ip.protocolDataSize(), ip.source(), ip.destination(), timestamp);
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
        TCPSimulCryptDump(Options& opt) : SimulCryptDump(opt) {}

        // Dump the file, return true on success, false on error.
        bool dump(std::ostream&);

    private:
        ts::PcapStream _file {};
    };
}

// Dump the file, return true on success, false on error.
bool TCPSimulCryptDump::dump(std::ostream& out)
{
    // Open the pcap file.
    if (!_file.loadArgs(_opt) || !_file.open(_opt.input_file, _opt)) {
        return false;
    }

    // Set packet filters.
    _file.setBidirectionalFilter(_opt.source_filter, _opt.dest_filter);

    // Read all TCP sessions matching the source and destination.
    for (;;) {
        cn::microseconds timestamp = cn::microseconds::zero();
        ts::IPSocketAddress source;
        ts::ByteBlock data;

        // Read a message header from any source.
        // There must be 5 header bytes: version(1), type(2), length(2).
        // See ETSI TS 103 197, section 4.4.1.
        size_t size = 5;
        if (!_file.readTCP(source, data, size, timestamp, _opt)) {
            break;
        }
        if (size < 5) {
            _opt.error(u"truncated message: %s (%s -> %s)", ts::UString::Dump(data, ts::UString::SINGLE_LINE), source, _file.otherFilter(source));
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
        TCPSessionDump(Options& opt) : _opt(opt) {}

        // Dump the session in hexadecimal, return true on success, false on error.
        bool dump(std::ostream&);

        // Save the session in a binary file.
        bool save();

    private:
        Options& _opt;
        ts::PcapStream _file {};

        // Dump a message.
        void dumpMessage(std::ostream&, const ts::ByteBlock&, const ts::IPSocketAddress& src, const ts::IPSocketAddress& dst, cn::microseconds timestamp);
    };
}

// Dump a message.
void TCPSessionDump::dumpMessage(std::ostream& out, const ts::ByteBlock& data, const ts::IPSocketAddress& src, const ts::IPSocketAddress& dst, cn::microseconds timestamp)
{
    if (!data.empty()) {
        if (timestamp > cn::microseconds::zero()) {
            out << (ts::Time::UnixEpoch + timestamp) << ", ";
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
    if (!_file.loadArgs(_opt) || !_file.open(_opt.input_file, _opt)) {
        return false;
    }

    // Set packet filters.
    _file.setBidirectionalFilter(_opt.source_filter, _opt.dest_filter);

    ts::ByteBlock data;
    cn::microseconds data_timestamp = cn::microseconds::zero();
    ts::IPSocketAddress data_source;
    ts::IPSocketAddress data_dest;
    ts::ByteBlock buf;
    ts::IPSocketAddress buf_source;

    // Read all TCP sessions matching the source and destination.
    for (;;) {
        // Read byte by byte, to make sure the alternance between client and server traffic is clearly identified.
        buf.clear();
        buf_source.clear();
        size_t size = 1;
        cn::microseconds timestamp = cn::microseconds::zero();
        if (!_file.readTCP(buf_source, buf, size, timestamp, _opt)) {
            break;
        }
        if (data_timestamp <= cn::microseconds::zero()) {
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

// Save the session, return true on success, false on error.
bool TCPSessionDump::save()
{
    // Open the pcap file.
    if (!_file.loadArgs(_opt) || !_file.open(_opt.input_file, _opt)) {
        return false;
    }

    // Set packet filters.
    _file.setBidirectionalFilter(_opt.source_filter, _opt.dest_filter);

    // Open/create the output file.
    std::ofstream outfile;
    std::ostream* out = &outfile;
    bool ok = true;
    if (_opt.output_file.empty() || _opt.output_file == u"-") {
        // Use standard output.
        ok = SetBinaryModeStdout(_opt);
        out = &std::cout;
    }
    else {
        outfile.open(_opt.output_file.toUTF8(), std::ios::out | std::ios::binary);
        ok = bool(outfile);
        if (!ok) {
            _opt.error(u"error creating %s", _opt.output_file);
        }
    }

    constexpr size_t buffer_size = 0xFFFF;
    ts::ByteBlock data;
    cn::microseconds timestamp = cn::microseconds::zero();
    ts::IPSocketAddress source(_opt.source_filter);

    // Read all TCP sessions matching the source and destination.
    while (ok) {
        size_t size = buffer_size;
        ok = _file.readTCP(source, data, size, timestamp, _opt);
        if (size == 0) {
            break;
        }
        if (ok) {
            out->write(reinterpret_cast<const char*>(data.data()), data.size());
            ok = bool(outfile);
            if (!ok) {
                _opt.error(u"error writing %s", _opt.output_file);
            }
            data.clear();
        }
    }

    _file.close();
    return ok;
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
    std::ostream& out(opt.save_tcp ? std::cout : opt.pager.output(opt));

    if (opt.extract_tcp) {
        // TCP session dump.
        TCPSessionDump tcp(opt);
        status = tcp.dump(out);
    }
    else if (opt.save_tcp) {
        // TCP session save.
        TCPSessionDump tcp(opt);
        status = tcp.save();
    }
    else if (!opt.dvb_simulcrypt) {
        // Global file analysis by default.
        FileAnalysis dfa(opt);
        status = dfa.analyze(out);
    }
    else if (opt.protocols.find(ts::IP_SUBPROTO_UDP) == opt.protocols.end()) {
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
