//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Dump PSI/SI tables, as saved by tstables.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsDuckProtocol.h"
#include "tsTime.h"
#include "tsSectionFile.h"
#include "tsTablesDisplay.h"
#include "tsUDPReceiver.h"
#include "tsTablesLogger.h"
#include "tsIPProtocols.h"
#include "tsSysUtils.h"
#include "tsPagerArgs.h"
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

        ts::DuckContext       duck {this};               // TSDuck execution context.
        ts::TablesDisplay     display {duck};            // Options about displaying tables
        ts::PagerArgs         pager {true, true};        // Output paging options.
        ts::UDPReceiverArgs   udp {};                    // Options about receiving UDP tables
        ts::duck::Protocol    duck_protocol {};          // To analyze incoming UDP messages
        std::vector<fs::path> infiles {};                // Input file names
        ts::CRC32::Validation crc_validation = ts::CRC32::CHECK;  // Validation of CRC32 in input sections
        size_t                max_tables = 0;            // Max number of tables to dump.
        size_t                max_invalid_udp = 16;      // Max number of invalid UDP messages before giving up.
        bool                  no_encapsulation = false;  // Raw sections in UDP messages.
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Dump PSI/SI tables, as saved by tstables", u"[options] [filename ...]")
{
    duck.defineArgsForCAS(*this);
    duck.defineArgsForPDS(*this);
    duck.defineArgsForStandards(*this);
    duck.defineArgsForTimeReference(*this);
    duck.defineArgsForCharset(*this);
    pager.defineArgs(*this);
    display.defineArgs(*this);
    udp.defineArgs(*this, false, false);

    option(u"", 0, FILENAME);
    help(u"",
         u"Input binary section file. Several files can be specified. By default, without "
         u"file and without --ip-udp, the binary tables are read from the standard input.\n\n"
         u"With --ip-udp, no file shall be specified. Binary sections and tables are "
         u"received over UDP/IP as sent by the utility 'tstables' or the plugin 'tables'.");

    option(u"ignore-crc32");
    help(u"ignore-crc32",
         u"Do not check CRC32 of input sections. "
         u"This can be used to analyze sections with incorrect CRC32 but otherwise correct.");

    option(u"max-tables", 'x', UNSIGNED);
    help(u"max-tables",
         u"Maximum number of tables to dump. Stop logging tables when this limit is "
         u"reached. Useful with --ip-udp which never ends otherwise.");

    option(u"no-encapsulation", 0);
    help(u"no-encapsulation",
         u"With --ip-udp, receive the tables as raw binary messages in UDP packets. "
         u"By default, the tables are formatted into TLV messages.");

    analyze(argc, argv);

    duck.loadArgs(*this);
    pager.loadArgs(*this);
    display.loadArgs(duck, *this);
    udp.loadArgs(*this);

    getPathValues(infiles, u"");
    max_tables = intValue<size_t>(u"max-tables", std::numeric_limits<size_t>::max());
    no_encapsulation = present(u"no-encapsulation");
    crc_validation = present(u"ignore-crc32") ? ts::CRC32::IGNORE : ts::CRC32::CHECK;

    if (!infiles.empty() && udp.destination.hasPort()) {
        error(u"specify input files or --ip-udp, but not both");
    }

    exitOnError();
}


//----------------------------------------------------------------------------
// Dump sections from UDP. Return true on success.
//----------------------------------------------------------------------------

namespace {
    bool DumpUDP(Options& opt)
    {
        // Initialize UDP receiver.
        ts::UDPReceiver sock(opt);
        sock.setParameters(opt.udp);
        if (!sock.open(opt)) {
            return false;
        }

        bool ok = true;
        size_t invalid_msg = 0;
        ts::IPSocketAddress sender;
        ts::IPSocketAddress destination;
        ts::ByteBlock packet(ts::IP_MAX_PACKET_SIZE);
        ts::Time timestamp;
        ts::SectionPtrVector sections;

        // Redirect display on pager process or stdout only.
        opt.duck.setOutput(&opt.pager.output(opt), false);

        // Receive UDP packets.
        while (ok && opt.max_tables > 0) {

            // Wait for a UDP message
            size_t insize = 0;
            ok = sock.receive(packet.data(), packet.size(), insize, sender, destination, nullptr, opt);

            // Check packet.
            assert(insize <= packet.size());
            if (ok) {
                // Analyze sections in the packet.
                if (ts::TablesLogger::AnalyzeUDPMessage(opt.duck_protocol, packet.data(), insize, opt.no_encapsulation, sections, timestamp)) {

                    // Valid message, reset the number of consecutive invalid messages.
                    invalid_msg = 0;

                    // Check if a complete table is available.
                    ts::BinaryTable table(sections, false, false);
                    if (table.isValid()) {
                        // Complete table available, dump as a table.
                        opt.display.displayTable(table);
                        opt.display.out() << std::endl;
                        opt.max_tables--;
                    }
                    else {
                        // Complete table not available, dump as individual sections.
                        for (auto it = sections.begin(); opt.max_tables > 0 && it != sections.end(); ++it) {
                            if (*it != nullptr) {
                                opt.display.displaySection(**it);
                                opt.display.out() << std::endl;
                                opt.max_tables--;
                            }
                        }
                    }
                }
                else {
                    // Cannot analyze UDP message, invalid message.
                    opt.error(u"invalid section in UDP packet (%s)", opt.no_encapsulation ? u"raw sections, no encapsulation" : u"TLV messages");
                    if (++invalid_msg >= opt.max_invalid_udp) {
                        opt.error(u"received too many consecutive invalid messages, giving up");
                        ok = false;
                    }
                }
            }
        }

        // Terminate UDP reception.
        ok = sock.close(opt) && ok;
        return ok;
    }
}


//----------------------------------------------------------------------------
//  Dump sections in a file. Return true on success.
//----------------------------------------------------------------------------

namespace {
    bool DumpFile(Options& opt, const ts::UString& file_name)
    {
        // Report file name in case of multiple files
        if (opt.verbose() && opt.infiles.size() > 1) {
            opt.pager.output(opt) << "* File: " << file_name << std::endl << std::endl;
        }

        // Load all sections
        bool ok = false;
        ts::SectionFile file(opt.duck);
        file.setCRCValidation(opt.crc_validation);

        if (file_name.empty()) {
            // no input file specified, use standard input
            SetBinaryModeStdin(opt);
            ok = file.loadBinary(std::cin);
        }
        else {
            ok = file.loadBinary(file_name);
        }

        if (ok) {
            // Display all sections.
            opt.duck.setOutput(&opt.pager.output(opt), false);
            for (auto it = file.sections().begin(); opt.max_tables > 0 && it != file.sections().end(); ++it) {
                opt.display.displaySection(**it);
                opt.display.out() << std::endl;
                opt.max_tables--;
            }
        }

        return ok;
    }
}


//----------------------------------------------------------------------------
// Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Get command line options.
    Options opt(argc, argv);
    bool ok = true;

    // Dump files or network packets.
    opt.pager.output(opt) << std::endl;
    if (opt.udp.destination.hasPort()) {
        ok = DumpUDP(opt);
    }
    else if (opt.infiles.size() == 0) {
        ok = DumpFile(opt, u"");
    }
    else {
        for (const auto& it : opt.infiles) {
            ok = DumpFile(opt, it) && ok;
        }
    }

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
