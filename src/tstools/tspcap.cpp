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
//  Analysis tool for pcap and pcap-ng files.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsPcapFile.h"
#include "tsIPv4Packet.h"
#include "tsTime.h"
#include "tsBitRate.h"
TSDUCK_SOURCE;
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

        ts::UString input_file;
        size_t      first_packet;
        size_t      last_packet;
        bool        tcp_filter;
        bool        udp_filter;
        bool        others_filter;
        bool        print_summary;
        bool        list_streams;
        bool        print_intervals;
        ts::IPv4SocketAddress source_filter;
        ts::IPv4SocketAddress dest_filter;
        ts::MicroSecond       first_time_offset;
        ts::MicroSecond       last_time_offset;
        ts::MicroSecond       first_time;
        ts::MicroSecond       last_time;
        ts::MicroSecond       interval;

    private:
        // Get a date option and return it as micro-seconds since Unix epoch.
        ts::MicroSecond getDate(const ts::UChar* arg, ts::MicroSecond def_value);
    };
}

// Get a date option and return it as micro-seconds since Unix epoch.
ts::MicroSecond Options::getDate(const ts::UChar* arg, ts::MicroSecond def_value)
{
    ts::Time date;
    const ts::UString str(value(arg));
    if (str.empty()) {
        return def_value;
    }
    else if (!date.decode(str, ts::Time::ALL)) {
        error(u"invalid date \"%s\", use format \"YYYY/MM/DD:hh:mm:ss.mmm\"", {str});
        return def_value;
    }
    else if (date < ts::Time::UnixEpoch) {
        error(u"invalid date %s, must be after %s", {str, ts::Time::UnixEpoch});
        return def_value;
    }
    else {
        return (date - ts::Time::UnixEpoch) * ts::MicroSecPerMilliSec;
    }
}

// Get command line options.
Options::Options(int argc, char *argv[]) :
    ts::Args(u"Analyze pcap and pcap-ng files", u"[options] [input-file]"),
    input_file(),
    first_packet(0),
    last_packet(0),
    tcp_filter(false),
    udp_filter(false),
    others_filter(false),
    print_summary(false),
    list_streams(false),
    print_intervals(false),
    source_filter(),
    dest_filter(),
    first_time_offset(-1),
    last_time_offset(-1),
    first_time(-1),
    last_time(-1),
    interval(-1)
 {
    option(u"", 0, STRING, 0, 1);
    help(u"", u"file-name",
         u"Input file in pcap or pcap-ng format, typically as saved by Wireshark. "
         u"Use the standard input if no file name is specified.");

    option(u"destination", 'd', STRING);
    help(u"destination", u"[address][:port]",
         u"Filter IPv4 packets based on the specified destination socket address. "
         u"The optional port number is used for TCP and UDP packets only.");

    option(u"first-packet", 0, POSITIVE);
    help(u"first-packet",
         u"Filter packets starting at the specified number. "
         u"The packet numbering counts all captured packets from the beginning of the file, starting at 1. "
         u"This is the same value as seen on Wireshark in the leftmost column.");

    option(u"first-timestamp", 0, UNSIGNED);
    help(u"first-timestamp", u"micro-seconds",
         u"Filter packets starting at the specified timestamp in micro-seconds from the beginning of the capture. "
         u"This is the same value as seen on Wireshark in the \"Time\" column (in seconds).");

    option(u"first-date", 0, STRING);
    help(u"first-date", u"date-time",
         u"Filter packets starting at the specified date. Use format YYYY/MM/DD:hh:mm:ss.mmm.");

    option(u"interval", 'i', POSITIVE);
    help(u"interval", u"micro-seconds",
         u"Print a summary of exchanged data by intervals of times in micro-seconds.");

    option(u"last-packet", 0, POSITIVE);
    help(u"last-packet",
         u"Filter packets up to the specified number. "
         u"The packet numbering counts all captured packets from the beginning of the file, starting at 1. "
         u"This is the same value as seen on Wireshark in the leftmost column.");

    option(u"last-timestamp", 0, UNSIGNED);
    help(u"last-timestamp", u"micro-seconds",
         u"Filter packets up to the specified timestamp in micro-seconds from the beginning of the capture. "
         u"This is the same value as seen on Wireshark in the \"Time\" column (in seconds).");

    option(u"last-date", 0, STRING);
    help(u"last-date", u"date-time",
         u"Filter packets up to the specified date. Use format YYYY/MM/DD:hh:mm:ss.mmm.");

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
    getValue(input_file, u"");
    const ts::UString dest_string(value(u"destination"));
    const ts::UString source_string(value(u"source"));
    tcp_filter = present(u"tcp");
    udp_filter = present(u"udp");
    others_filter = present(u"others");
    getIntValue(first_packet, u"first-packet", 1);
    getIntValue(last_packet, u"last-packet", std::numeric_limits<size_t>::max());
    getIntValue(first_time_offset, u"first-timestamp", 0);
    getIntValue(last_time_offset, u"last-timestamp", std::numeric_limits<ts::MicroSecond>::max());
    getIntValue(interval, u"interval", 0);
    first_time = getDate(u"first-date", 0);
    last_time = getDate(u"last-date", std::numeric_limits<ts::MicroSecond>::max());
    list_streams = present(u"list-streams");
    print_intervals = present(u"interval");

    // Default is to print a summary of the file content.
    print_summary = !list_streams && !print_intervals;

    // Default is to filter all protocols.
    if (!tcp_filter && !udp_filter && !others_filter) {
        tcp_filter = udp_filter = others_filter = true;
    }

    // Decode network addresses.
    if (!source_string.empty()) {
        source_filter.resolve(source_string, *this);
    }
    if (!dest_string.empty()) {
        dest_filter.resolve(dest_string, *this);
    }

    // Final checking
    exitOnError();
}


//----------------------------------------------------------------------------
// Read next IPv4 packet matching input filters.
//----------------------------------------------------------------------------

namespace {
    bool ReadPacket(Options& opt, ts::PcapFile& file, ts::IPv4Packet& ip, ts::MicroSecond& timestamp)
    {
        while (file.readIPv4(ip, timestamp, opt)) {
            // Do not read before --first-packet and after --last-packet.
            if (file.packetCount() < opt.first_packet ||
                timestamp < opt.first_time ||
                file.timeOffset(timestamp) < opt.first_time_offset)
            {
                continue;
            }
            if (file.packetCount() > opt.last_packet ||
                timestamp > opt.last_time ||
                file.timeOffset(timestamp) > opt.last_time_offset)
            {
                return false;
            }
            // Check if the packet shall be analyzed.
            if ((!ip.isTCP() || opt.tcp_filter) &&
                (!ip.isUDP() || opt.udp_filter) &&
                (ip.isUDP() || ip.isTCP() || opt.others_filter) &&
                opt.source_filter.match(ip.sourceSocketAddress()) &&
                opt.dest_filter.match(ip.destinationSocketAddress()))
            {
                opt.debug(u"packet: ip size: %'d, data size: %'d, timestamp: %'d", {ip.size(), ip.protocolDataSize(), timestamp});
                return true;
            }
        }
        return false;
    }
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
// Display summary of content.
//----------------------------------------------------------------------------

namespace {
    void DisplaySummary(std::ostream& out, const ts::PcapFile& file, const StatBlock& stats)
    {
        const size_t hwidth = 22; // header width

        out << std::endl;
        out << "File summary:" << std::endl;
        out << ts::UString::Format(u"  %-*s %s", {hwidth, u"File:", file.fileName()}) << std::endl;
        out << ts::UString::Format(u"  %-*s %'d", {hwidth, u"Total packets in file:", file.packetCount()}) << std::endl;
        out << ts::UString::Format(u"  %-*s %'d", {hwidth, u"Total IPv4 packets:", file.ipv4PacketCount()}) << std::endl;
        out << ts::UString::Format(u"  %-*s %'d bytes", {hwidth, u"File size:", file.fileSize()}) << std::endl;
        out << ts::UString::Format(u"  %-*s %'d bytes", {hwidth, u"Total packets size:", file.totalPacketsSize()}) << std::endl;
        out << ts::UString::Format(u"  %-*s %'d bytes", {hwidth, u"Total IPv4 size:", file.totalIPv4PacketsSize()}) << std::endl;
        out << std::endl;

        out << "Filtered packets summary:" << std::endl;
        out << ts::UString::Format(u"  %-*s %'d", {hwidth, u"Packets:", stats.packet_count}) << std::endl;
        out << ts::UString::Format(u"  %-*s %'d", {hwidth, u"Packets size:", stats.total_ip_size}) << std::endl;
        out << ts::UString::Format(u"  %-*s %'d", {hwidth, u"Payload data size:", stats.total_data_size}) << std::endl;

        if (stats.first_timestamp > 0 && stats.last_timestamp > 0) {
            const ts::Time start(ts::Time::UnixEpoch + stats.first_timestamp / ts::MicroSecPerMilliSec);
            const ts::Time end(ts::Time::UnixEpoch + stats.last_timestamp / ts::MicroSecPerMilliSec);
            const ts::MicroSecond duration = stats.last_timestamp - stats.first_timestamp;
            out << ts::UString::Format(u"  %-*s %s (%+'d micro-seconds)", {hwidth, u"Start time:", start, file.timeOffset(stats.first_timestamp)}) << std::endl;
            out << ts::UString::Format(u"  %-*s %s (%+'d micro-seconds)", {hwidth, u"End time:", end, file.timeOffset(stats.last_timestamp)}) << std::endl;
            if (duration > 0) {
                out << ts::UString::Format(u"  %-*s %'d micro-seconds", {hwidth, u"Duration:", duration}) << std::endl;
                out << ts::UString::Format(u"  %-*s %'d bits/second", {hwidth, u"IP bitrate:", (ts::BitRate(stats.total_ip_size * 8 * ts::MicroSecPerSec) / duration)}) << std::endl;
                out << ts::UString::Format(u"  %-*s %'d bits/second", {hwidth, u"Data bitrate:", (ts::BitRate(stats.total_data_size * 8 * ts::MicroSecPerSec) / duration)}) << std::endl;
            }
        }
        out << std::endl;
    }
}


//----------------------------------------------------------------------------
// Display list of streams.
//----------------------------------------------------------------------------

namespace {
    void ListStreams(std::ostream& out, const ts::PcapFile& file, const std::map<StreamId,StatBlock>& stats, ts::MicroSecond duration)
    {
        out << std::endl;
        out << ts::UString::Format(u"%-22s %-22s %-8s %11s %15s %11s",
                                   {u"Source", u"Destination", u"Protocol", u"Packets", u"Data bytes", u"Bitrate"})
            << std::endl;
        for (auto it = stats.begin(); it != stats.end(); ++it) {
            const StreamId& id(it->first);
            const StatBlock& sb(it->second);
            out << ts::UString::Format(u"%-22s %-22s %-8s %11'd %15'd %11'd",
                                       {id.source, id.destination, ts::IPProtocolName(id.protocol),
                                        sb.packet_count, sb.total_data_size,
                                        duration <= 0 ? 0 : (ts::BitRate(sb.total_data_size * 8 * ts::MicroSecPerSec) / duration)})
                << std::endl;
        }
        out << std::endl;
    }
}


//----------------------------------------------------------------------------
// Display summary of content by interval of time.
//----------------------------------------------------------------------------

namespace {
    class DisplayInterval
    {
        TS_NOBUILD_NOCOPY(DisplayInterval);
    public:
        // Constructor.
        DisplayInterval(Options& opt);

        // Start, stop display.
        void start(std::ostream& out);
        void stop(std::ostream& out);

        // Process one IPv4 packet.
        void addPacket(std::ostream& out, const ts::IPv4Packet&, ts::MicroSecond);

    private:
        Options&  _opt;
        StatBlock _stat;
    };
}

// Constructor.
DisplayInterval::DisplayInterval(Options& opt) :
    _opt(opt),
    _stat()
{
}

// Start the display.
void DisplayInterval::start(std::ostream& out)
{
    out << std::endl;
    //@@@@
}

// Process one IPv4 packet.
void DisplayInterval::addPacket(std::ostream& out, const ts::IPv4Packet&, ts::MicroSecond)
{
    if (_opt.interval > 0) {
        //@@@@
    }
}

// Stop the display.
void DisplayInterval::stop(std::ostream& out)
{
    //@@@@
    out << std::endl;
}


//----------------------------------------------------------------------------
// Program main code.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Get command line options.
    Options opt(argc, argv);

    // Open the pcap file.
    ts::PcapFile file;
    if (!file.open(opt.input_file, opt)) {
        return EXIT_FAILURE;
    }

    // Statistics per data stream and global.
    std::map<StreamId,StatBlock> streams_stats;
    StatBlock global_stats;

    // Display list of time intervals.
    DisplayInterval interval(opt);
    if (opt.print_intervals) {
        interval.start(std::cout);
    }

    // Read all IPv4 packets from the file.
    ts::IPv4Packet ip;
    ts::MicroSecond timestamp = 0;
    while (ReadPacket(opt, file, ip, timestamp)) {
        global_stats.addPacket(ip, timestamp);
        if (opt.list_streams) {
            streams_stats[StreamId(ip.sourceSocketAddress(), ip.destinationSocketAddress(), ip.protocol())].addPacket(ip, timestamp);
        }
        if (opt.print_intervals) {
            interval.addPacket(std::cout, ip, timestamp);
        }
    }
    file.close();

    // Print final data.
    if (opt.print_intervals) {
        interval.stop(std::cout);
    }
    if (opt.list_streams) {
        ListStreams(std::cout, file, streams_stats, global_stats.last_timestamp - global_stats.first_timestamp);
    }
    if (opt.print_summary) {
        DisplaySummary(std::cout, file, global_stats);
    }
    return EXIT_SUCCESS;
}
