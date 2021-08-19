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
#include "tsIPv4SocketAddress.h"
#include "tsIPUtils.h"
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

        ts::UString       input_file;
        ts::IPv4SocketAddress source_filter;
        ts::IPv4SocketAddress dest_filter;
        size_t            first_packet;
        size_t            last_packet;
        bool              tcp_filter;
        bool              udp_filter;
        bool              others_filter;
    };
}

Options::Options(int argc, char *argv[]) :
    ts::Args(u"Analyze pcap and pcap-ng files", u"[options] [input-file]"),
    input_file(),
    source_filter(),
    dest_filter(),
    first_packet(0),
    last_packet(0),
    tcp_filter(false),
    udp_filter(false),
    others_filter(false)
{
    option(u"", 0, STRING, 0, 1);
    help(u"", u"file-name",
         u"Input file in pcap or pcap-ng format, typically as saved by Wireshark. "
         u"Use the standard input if no file name is specified.");

    option(u"destination", 'd', STRING);
    help(u"destination", u"[address][:port]",
         u"Filter IPv4 packets based on the specified destination socket address. "
         u"The optional port number is used for TCP and UDP packets only.");

    option(u"first-packet", 'f', POSITIVE);
    help(u"first-packet",
         u"Filter packets starting at the specified number. "
         u"The packet numbering counts all captured packets from the beginning of the file, starting at 1. "
         u"This is the same value as seen on Wireshark in the leftmost column.");

    option(u"last-packet", 'l', POSITIVE);
    help(u"last-packet",
         u"Filter packets up to the specified number. "
         u"The packet numbering counts all captured packets from the beginning of the file, starting at 1. "
         u"This is the same value as seen on Wireshark in the leftmost column.");

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
// Check if an IPv4 packet is valid and matches input filters.
// Return true if the packet must be analyzed.
// Return offset of payload (IP, TCP or UDP).
//----------------------------------------------------------------------------

namespace {
    bool FilterPacket(Options& opt, const uint8_t* ip, size_t ip_size, size_t& payload_offset)
    {
        uint8_t proto = 0;
        size_t ip_header_size = 0;
        size_t proto_header_size = 0;
        ts::IPv4SocketAddress source;
        ts::IPv4SocketAddress destination;

        // Validate the IP packet.
        if (!ts::AnalyzeIPPacket(ip, ip_size, proto, ip_header_size, proto_header_size, source, destination)) {
            payload_offset = 0;
            return false;
        }

        // Validate the filters.
        payload_offset = ip_header_size + proto_header_size;
        return  (proto != ts::IPv4_PROTO_TCP || opt.tcp_filter) &&
                (proto != ts::IPv4_PROTO_UDP || opt.udp_filter) &&
                (proto == ts::IPv4_PROTO_UDP || proto == ts::IPv4_PROTO_TCP || opt.others_filter) &&
                opt.source_filter.match(source) &&
                opt.dest_filter.match(destination);
    }
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

    // Statistics data.
    size_t packet_count = 0;
    size_t total_ip_size = 0;
    size_t total_data_size = 0;
    ts::MicroSecond first_timestamp = -1;
    ts::MicroSecond last_timestamp = -1;

    // Read all IPv4 packets from the file.
    std::array<uint8_t, ts::IP_MAX_PACKET_SIZE> packet;
    size_t ip_size = 0;
    size_t payload_offset = 0;
    ts::MicroSecond timestamp = 0;
    while (file.readIPv4(packet.data(), packet.size(), ip_size, timestamp, opt)) {
        // Do not read after --last-packet.
        if (file.packetCount() > opt.last_packet) {
            break;
        }
        // Check if the packet shall be analyzed.
        if (file.packetCount() >= opt.first_packet && FilterPacket(opt, packet.data(), ip_size, payload_offset)) {
            opt.debug(u"packet: ip size: %'d, payload offset: %'d, timestamp: %'d", {ip_size, payload_offset, timestamp});
            assert(payload_offset <= ip_size);
            packet_count++;
            total_ip_size += ip_size;
            total_data_size += ip_size - payload_offset;
            if (timestamp >= 0) {
                if (first_timestamp < 0) {
                    first_timestamp = timestamp;
                }
                last_timestamp = timestamp;
            }
        }
    }

    // Display statistics.
    std::cout << ts::UString::Format(u"IPv4 packets: %'d (out of %'d)", {packet_count, file.packetCount()}) << std::endl;
    std::cout << ts::UString::Format(u"Total packets size: %'d bytes", {total_ip_size}) << std::endl;
    std::cout << ts::UString::Format(u"Total data size: %'d bytes", {total_data_size}) << std::endl;
    if (first_timestamp < 0 || last_timestamp < 0) {
        std::cout << "No timestamp available" << std::endl;
    }
    else {
        ts::Time start(ts::Time::UnixEpoch + first_timestamp / ts::MicroSecPerMilliSec);
        ts::Time end(ts::Time::UnixEpoch + last_timestamp / ts::MicroSecPerMilliSec);
        std::cout << "Start time: " << start << std::endl;
        std::cout << "End time:   " << end << std::endl;
        const ts::MicroSecond duration = last_timestamp - first_timestamp;
        if (duration > 0) {
            std::cout << ts::UString::Format(u"Duration: %'d micro-seconds", {duration}) << std::endl;
            std::cout << "IP bitrate:   " << (ts::BitRate(total_ip_size * 8 * ts::MicroSecPerSec) / duration) << " b/s" << std::endl;
            std::cout << "Data bitrate: " << (ts::BitRate(total_data_size * 8 * ts::MicroSecPerSec) / duration) << " b/s" << std::endl;
        }
    }

    file.close();
    return EXIT_SUCCESS;
}
