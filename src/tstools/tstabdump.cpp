//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
#include "tsTime.h"
#include "tsSectionFile.h"
#include "tsTablesDisplay.h"
#include "tsUDPReceiver.h"
#include "tsTablesLogger.h"
#include "tsCASFamily.h"
#include "tsSection.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;

// With static link, enforce a reference to MPEG/DVB structures.
#if defined(TSDUCK_STATIC_LIBRARY)
#include "tsStaticReferencesDVB.h"
const ts::StaticReferencesDVB dependenciesForStaticLib;
#endif


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public ts::Args
{
    Options(int argc, char *argv[]);

    ts::UStringVector     infiles;           // Input file names
    ts::TablesDisplayArgs display;           // Options about displaying tables
    ts::UDPReceiver       udp;               // Options about receiving UDP tables
    size_t                max_tables;        // Max number of tables to dump.
    size_t                max_invalid_udp;   // Max number of invalid UDP messages before giving up.
    bool                  no_encapsulation;  // Raw sections in UDP messages.
};

Options::Options(int argc, char *argv[]) :
    Args(u"Dump PSI/SI tables, as saved by tstables", u"[options] [filename ...]"),
    infiles(),
    display(),
    udp(*this, false, false),
    max_tables(0),
    max_invalid_udp(16),
    no_encapsulation(false)
{
    option(u"", 0, STRING);
    help(u"",
         u"Input binary section file. Several files can be specified. By default, without "
         u"file and without --ip-udp, the binary tables are read from the standard input.\n\n"
         u"With --ip-udp, no file shall be specified. Binary sections and tables are "
         u"received over UDP/IP as sent by the utility 'tstables' or the plugin 'tables'.");

    option(u"max-tables", 'x', UNSIGNED);
    help(u"max-tables",
         u"Maximum number of tables to dump. Stop logging tables when this limit is "
         u"reached. Useful with --ip-udp which never ends otherwise.");

    option(u"no-encapsulation", 0);
    help(u"no-encapsulation",
         u"With --ip-udp, receive the tables as raw binary messages in UDP packets. "
         u"By default, the tables are formatted into TLV messages.");

    // Additional options for display tables and to receive from UDP.
    display.defineOptions(*this);
    udp.defineOptions(*this);

    analyze(argc, argv);

    getValues(infiles, u"");
    display.load(*this);
    udp.load(*this);
    max_tables = intValue<size_t>(u"max-tables", std::numeric_limits<size_t>::max());
    no_encapsulation = present(u"no-encapsulation");

    if (!infiles.empty() && udp.receiverSpecified()) {
        error(u"specify input files or --ip-udp, but not both");
    }

    exitOnError();
}


//----------------------------------------------------------------------------
//  Dump sections from UDP. Return true on success.
//----------------------------------------------------------------------------

bool DumpUDP(Options& opt)
{
    // Initialize UDP receiver.
    if (!opt.udp.open(opt)) {
        return false;
    }

    bool ok = true;
    size_t invalid_msg = 0;
    ts::SocketAddress sender;
    ts::SocketAddress destination;
    ts::ByteBlock packet(ts::IP_MAX_PACKET_SIZE);
    ts::TablesDisplay display(opt.display, opt);
    ts::Time timestamp;
    ts::SectionPtrVector sections;

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
                    display.displayTable(table) << std::endl;
                    opt.max_tables--;
                }
                else {
                    // Complete table not available, dump as individual sections.
                    for (ts::SectionPtrVector::const_iterator it = sections.begin(); opt.max_tables > 0 && it != sections.end(); ++it) {
                        if (!it->isNull()) {
                            display.displaySection(**it) << std::endl;
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


//----------------------------------------------------------------------------
//  Dump sections in a file. Return true on success.
//----------------------------------------------------------------------------

bool DumpFile(Options& opt, const ts::UString& file_name)
{
    // Report file name in case of multiple files
    if (opt.verbose() && opt.infiles.size() > 1) {
        std::cout << "* File: " << file_name << std::endl << std::endl;
    }

    // Load all sections
    bool ok = false;
    ts::SectionFile file;
    file.setDefaultCharset(opt.display.default_charset);

    if (file_name.empty()) {
        // no input file specified, use standard input
        SetBinaryModeStdin(opt);
        ok = file.loadBinary(std::cin, opt);
    }
    else {
        ok = file.loadBinary(file_name, opt);
    }

    if (ok) {
        // Display all sections.
        ts::TablesDisplay display(opt.display, opt);
        for (ts::SectionPtrVector::const_iterator it = file.sections().begin(); opt.max_tables > 0 && it != file.sections().end(); ++it) {
            display.displaySection(**it) << std::endl;
            opt.max_tables--;
        }
    }

    return ok;
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
    std::cout << std::endl;
    if (opt.udp.receiverSpecified()) {
        ok = DumpUDP(opt);
    }
    else if (opt.infiles.size() == 0) {
        ok = DumpFile(opt, u"");
    }
    else {
        for (ts::UStringVector::const_iterator it = opt.infiles.begin(); it != opt.infiles.end(); ++it) {
            ok = DumpFile(opt, *it) && ok;
        }
    }

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

TS_MAIN(MainCode)
