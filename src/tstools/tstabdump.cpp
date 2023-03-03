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
//  Dump PSI/SI tables, as saved by tstables.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsTime.h"
#include "tsSectionFile.h"
#include "tsTablesDisplay.h"
#include "tsUDPReceiver.h"
#include "tsTablesLogger.h"
#include "tsSection.h"
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

        ts::DuckContext       duck;              // TSDuck execution context.
        ts::TablesDisplay     display;           // Options about displaying tables
        ts::PagerArgs         pager;             // Output paging options.
        ts::UDPReceiver       udp;               // Options about receiving UDP tables
        ts::UStringVector     infiles;           // Input file names
        ts::CRC32::Validation crc_validation;    // Validation of CRC32 in input sections
        size_t                max_tables;        // Max number of tables to dump.
        size_t                max_invalid_udp;   // Max number of invalid UDP messages before giving up.
        bool                  no_encapsulation;  // Raw sections in UDP messages.
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Dump PSI/SI tables, as saved by tstables", u"[options] [filename ...]"),
    duck(this),
    display(duck),
    pager(true, true),
    udp(*this),
    infiles(),
    crc_validation(ts::CRC32::CHECK),
    max_tables(0),
    max_invalid_udp(16),
    no_encapsulation(false)
{
    duck.defineArgsForCAS(*this);
    duck.defineArgsForPDS(*this);
    duck.defineArgsForStandards(*this);
    duck.defineArgsForTimeReference(*this);
    duck.defineArgsForCharset(*this);
    pager.defineArgs(*this);
    display.defineArgs(*this);
    udp.defineArgs(*this, false, false, false);

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
    pager.loadArgs(duck, *this);
    display.loadArgs(duck, *this);
    udp.loadArgs(duck, *this);

    getValues(infiles, u"");
    max_tables = intValue<size_t>(u"max-tables", std::numeric_limits<size_t>::max());
    no_encapsulation = present(u"no-encapsulation");
    crc_validation = present(u"ignore-crc32") ? ts::CRC32::IGNORE : ts::CRC32::CHECK;

    if (!infiles.empty() && udp.receiverSpecified()) {
        error(u"specify input files or --ip-udp, but not both");
    }

    exitOnError();
}


//----------------------------------------------------------------------------
//  Dump sections from UDP. Return true on success.
//----------------------------------------------------------------------------

namespace {
    bool DumpUDP(Options& opt)
    {
        // Initialize UDP receiver.
        if (!opt.udp.open(opt)) {
            return false;
        }

        bool ok = true;
        size_t invalid_msg = 0;
        ts::IPv4SocketAddress sender;
        ts::IPv4SocketAddress destination;
        ts::ByteBlock packet(ts::IP_MAX_PACKET_SIZE);
        ts::Time timestamp;
        ts::SectionPtrVector sections;

        // Redirect display on pager process or stdout only.
        opt.duck.setOutput(&opt.pager.output(opt), false);

        // Receive UDP packets.
        while (ok && opt.max_tables > 0) {

            // Wait for a UDP message
            size_t insize = 0;
            ok = opt.udp.receive(packet.data(), packet.size(), insize, sender, destination, nullptr, opt);

            // Check packet.
            assert(insize <= packet.size());
            if (ok) {
                // Analyze sections in the packet.
                if (ts::TablesLogger::AnalyzeUDPMessage(packet.data(), insize, opt.no_encapsulation, sections, timestamp)) {

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
                            if (!it->isNull()) {
                                opt.display.displaySection(**it);
                                opt.display.out() << std::endl;
                                opt.max_tables--;
                            }
                        }
                    }
                }
                else {
                    // Cannot analyze UDP message, invalid message.
                    opt.error(u"invalid section in UDP packet (%s)", {opt.no_encapsulation ? u"raw sections, no encapsulation" : u"TLV messages"});
                    if (++invalid_msg >= opt.max_invalid_udp) {
                        opt.error(u"received too many consecutive invalid messages, giving up");
                        ok = false;
                    }
                }
            }
        }

        // Terminate UDP reception.
        ok = opt.udp.close(opt) && ok;
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
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Get command line options.
    Options opt(argc, argv);
    bool ok = true;

    // Dump files or network packets.
    opt.pager.output(opt) << std::endl;
    if (opt.udp.receiverSpecified()) {
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
