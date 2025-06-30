//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Dump the packets from a transport stream.
//  Also generic hexa/ascii dump utility (option --raw).
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsTSPacket.h"
#include "tsTSFile.h"
#include "tsTSDumpArgs.h"
#include "tsPagerArgs.h"
#include "tsDuckContext.h"
#include "tsUDPReceiver.h"
#include "tsIPProtocols.h"
#include "tsArgs.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace {
    class Options: public ts::Args
    {
        TS_NOBUILD_NOCOPY(Options);
    public:
        Options(int argc, char *argv[]);

        ts::DuckContext     duck {this};         // TSDuck context
        bool                raw_file = false;    // Raw dump of file, not TS packets
        bool                udp_dump = false;    // Dump UDP packets, not TS packets
        uint32_t            raw_flags = 0;       // Raw dump flags
        size_t              raw_bpl = 0;         // Bytes per line in raw mode.
        uint64_t            start_offset = 0;    // Start offset in bytes
        ts::PacketCounter   max_packets = 0;     // Maximum number of packets to dump per file
        ts::UStringVector   infiles {};          // Input file names
        ts::TSPacketFormat  format = ts::TSPacketFormat::AUTODETECT;  // Input file format
        ts::TSDumpArgs      dump {};             // Packet dump options
        ts::PagerArgs       pager {true, true};  // Output paging options
        ts::UDPReceiverArgs udp {};              // UDP options
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Dump and format MPEG transport stream packets", u"[options] [filename ...]")
{
    duck.defineArgsForStandards(*this);
    udp.defineArgs(*this, false, false);
    dump.defineArgs(*this);
    pager.defineArgs(*this);
    ts::DefineTSPacketFormatInputOption(*this, 'f');

    option(u"", 0, FILENAME, 0, UNLIMITED_COUNT);
    help(u"", u"Any number of input MPEG TS files (standard input if omitted).");

    option(u"byte-offset", 0, UNSIGNED);
    help(u"byte-offset",
         u"Start reading each file at the specified byte offset (default: 0). "
         u"This option is allowed only if all input files are regular files.");

    option(u"c-style", 'c');
    help(u"c-style", u"Same as --raw-dump (no interpretation of packet) but dump the bytes in C-language style.");

    option(u"max-packets", 'm', UNSIGNED);
    help(u"max-packets", u"Maximum number of packets to dump per file.");

    option(u"packet-offset", 0, UNSIGNED);
    help(u"packet-offset",
         u"Start reading each file at the specified TS packet (default: 0). "
         u"This option is allowed only if all input files are regular files.");

    option(u"raw-file", 'r');
    help(u"raw-file", u"Raw dump of file, do not interpret as TS packets.");

    analyze(argc, argv);

    duck.loadArgs(*this);
    udp.loadArgs(*this);
    dump.loadArgs(duck, *this);
    pager.loadArgs(*this);

    getValues(infiles);
    raw_file = present(u"raw-file");
    start_offset = intValue<uint64_t>(u"byte-offset", intValue<uint64_t>(u"packet-offset", 0) * ts::PKT_SIZE);
    getIntValue(max_packets, u"max-packets", std::numeric_limits<ts::PacketCounter>::max());
    format = ts::LoadTSPacketFormatInputOption(*this);

    if (present(u"c-style")) {
        dump.dump_flags |= ts::UString::C_STYLE;
        raw_file = true;
    }

    // Receiving from UDP means --raw-file, without files.
    udp_dump = udp.destination.hasPort();
    raw_file = raw_file || udp_dump;
    if (udp_dump && !infiles.empty()) {
        error(u"don't specify input files with --ip-udp");
    }

    // Filter TS-specific options when used with --raw-file.
    if (raw_file && (dump.log || (dump.pids.any() && !dump.pids.all()))) {
        error(u"--raw-file and --ip-udp are incompatible with TS-specific options --pid and --log");
    }

    // Dump flags for raw mode.
    if (raw_file) {
        raw_flags = (dump.dump_flags & 0x0000FFFF) | ts::UString::BPL | ts::UString::WIDE_OFFSET;
        raw_bpl = (raw_flags & ts::UString::BINARY) ? 8 : 16;
    }

    exitOnError();
}


//----------------------------------------------------------------------------
// Perform the dump on one transport stream file.
//----------------------------------------------------------------------------

namespace {
    void DumpTSFile(Options& opt, const ts::UString& filename, std::ostream& out)
    {
        if (opt.infiles.size() > 1 && !opt.dump.log) {
            out << "* File " << filename << std::endl;
        }

        // Open the TS file.
        ts::TSFile file;
        if (!file.openRead(filename, 1, opt.start_offset, opt, opt.format)) {
            return;
        }

        // Read all packets in the file.
        // Stop on output error (typically 'quit' in the pager).
        ts::TSPacket pkt;
        ts::TSPacketMetadata mdata;
        for (ts::PacketCounter packet_index = 0; out && packet_index < opt.max_packets && file.readPackets(&pkt, &mdata, 1, opt) > 0; packet_index++) {
            if (opt.dump.pids.test(pkt.getPID())) {
                if (!opt.dump.log) {
                    out << std::endl << "* Packet " << ts::UString::Decimal(packet_index) << std::endl;
                }
                opt.dump.dump(opt.duck, out, pkt, &mdata);
            }
        }
        file.close(opt);

        if (!opt.dump.log) {
            out << std::endl;
        }
    }
}


//----------------------------------------------------------------------------
// Perform the raw dump on one input file.
//----------------------------------------------------------------------------

namespace {
    void DumpRawFile(Options& opt, const ts::UString& filename, std::ostream& out)
    {
        std::istream* in = nullptr;
        std::ifstream file;

        // Open input file (standard input if no file is specified or file name is empty).
        if (filename.empty() || filename == u"-") {
            // Use standard input.
            in = &std::cin;
            // Try to put standard input in binary mode
            ts::SetBinaryModeStdin(opt);
        }
        else {
            // Dump named files. Open the file in binary mode. Will be closed by destructor.
            in = &file;
            file.open(filename.toUTF8().c_str(), std::ios::binary);
            if (!file) {
                opt.error(u"cannot open file %s", filename);
                return;
            }
        }

        // Raw dump of file
        // Stop on output error (typically 'quit' in the pager).
        ts::ByteBlock buffer(opt.raw_bpl);
        size_t offset = 0;
        size_t size = 0;
        int c = EOF;
        while (*in && out) {
            for (size = 0; size < buffer.size() && (c = in->get()) != EOF; size++) {
                buffer[size] = uint8_t(c);
            }
            out << ts::UString::Dump(buffer.data(), size, opt.raw_flags, 0, opt.raw_bpl, offset);
            offset += size;
        }
    }
}


//----------------------------------------------------------------------------
// Perform the raw dump on UDP packets.
//----------------------------------------------------------------------------

namespace {
    void DumpRawUDP(Options& opt, std::ostream& out)
    {
        // Initializ◊e the UDP reception.
        ts::UDPReceiver sock(opt);
        sock.setParameters(opt.udp);
        if (!sock.open(opt)) {
            return;
        }

        // Raw dump of all received datagrams.
        // Stop on output error (typically 'quit' in the pager).
        ts::ByteBlock buffer(ts::IP_MAX_PACKET_SIZE);
        size_t size = 0;
        ts::IPSocketAddress sender;
        ts::IPSocketAddress destination;
        const bool headers = opt.dump.dump_flags & ts::TSPacket::DUMP_TS_HEADER;

        for (ts::PacketCounter packet_index = 0;
             out && packet_index < opt.max_packets && sock.receive(buffer.data(), buffer.size(), size, sender, destination, nullptr, opt);
             packet_index++)
        {
            if (headers) {
                out << std::endl
                    << "* Packet " << ts::UString::Decimal(packet_index)
                    << ", " << ts::UString::Decimal(size) << " bytes, "
                    << sender << " -> " << destination
                    << std::endl;
            }
            out << ts::UString::Dump(buffer.data(), size, opt.raw_flags, 0, opt.raw_bpl);
        }
        sock.close(opt);
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Decode command line.
    Options opt(argc, argv);

    // Setup an output pager if necessary.
    std::ostream& out(opt.pager.output(opt));

    if (opt.udp_dump) {
        // Dump UDP packets.
        DumpRawUDP(opt, out);
    }
    else if (opt.infiles.empty()) {
        // Dump standard input.
        if (opt.raw_file) {
            DumpRawFile(opt, ts::UString(), out);
        }
        else {
            DumpTSFile(opt, ts::UString(), out);
        }
    }
    else {
        // Dump named files.
        for (const auto& name : opt.infiles) {
            if (opt.raw_file) {
                DumpRawFile(opt, name, out);
            }
            else {
                DumpTSFile(opt, name, out);
            }
        }
    }

    return opt.valid() ? EXIT_SUCCESS : EXIT_FAILURE;
}
