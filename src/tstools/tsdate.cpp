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
//  Display date & time information (TDT & TOT) from a transport stream
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsTSFile.h"
#include "tsTablesDisplay.h"
#include "tsSectionDemux.h"
#include "tsBinaryTable.h"
#include "tsTSPacket.h"
#include "tsNames.h"
#include "tsTDT.h"
#include "tsTOT.h"
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

        ts::DuckContext    duck;     // TSDuck execution context.
        ts::TablesDisplay  display;  // Table formatting options (all default values, nothing on command line).
        bool               no_tdt;   // Do not try to get a TDT
        bool               no_tot;   // Do not try to get a TOT
        bool               all;      // Report all tables, not only the first one.
        ts::UString        infile;   // Input file name
        ts::TSPacketFormat format;   // Input file format.
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Extract the date and time (TDT/TOT) from a transport stream", u"[options] [filename]"),
    duck(this),
    display(duck),
    no_tdt(false),
    no_tot(false),
    all(false),
    infile(),
    format(ts::TSPacketFormat::AUTODETECT)
{
    duck.defineArgsForStandards(*this);
    duck.defineArgsForTimeReference(*this);
    ts::DefineTSPacketFormatInputOption(*this, 'f');

    option(u"", 0, FILENAME, 0, 1);
    help(u"", u"MPEG capture file (standard input if omitted).");

    option(u"all", 'a');
    help(u"all", u"Report all TDT/TOT tables (default: report only the first table of each type).");

    option(u"notdt", 0);
    help(u"notdt", u"Ignore Time & Date Table (TDT).");

    option(u"notot", 0);
    help(u"notot", u"Ignore Time Offset Table (TOT).");

    analyze(argc, argv);

    duck.loadArgs(*this);

    infile = value(u"");
    all = present(u"all");
    no_tdt = present(u"notdt");
    no_tot = present(u"notot");
    format = ts::LoadTSPacketFormatInputOption(*this);

    exitOnError();
}


//----------------------------------------------------------------------------
//  Table handler: receives TOT and TDT
//----------------------------------------------------------------------------

class TableHandler: public ts::TableHandlerInterface
{
    TS_NOBUILD_NOCOPY(TableHandler);
public:
    // Constructor
    TableHandler(Options& opt) :
        _opt(opt),
        _tdt_ok(opt.no_tdt),
        _tot_ok(opt.no_tot)
    {
    }

    // Return true when analysis is complete
    bool completed() const
    {
        return _tdt_ok && _tot_ok;
    }

    // This hook is invoked when a complete table is available.
    virtual void handleTable(ts::SectionDemux&, const ts::BinaryTable&) override;

private:
    Options& _opt;
    bool     _tdt_ok;  // Finished TDT processing
    bool     _tot_ok;  // Finished TOT processing
};


//----------------------------------------------------------------------------
// This hook is invoked when a complete table is available.
//----------------------------------------------------------------------------

void TableHandler::handleTable(ts::SectionDemux&, const ts::BinaryTable& table)
{
    switch (table.tableId()) {

        case ts::TID_TDT: {
            if (_opt.no_tdt) {
                break;
            }
            _tdt_ok = !_opt.all;
            if (_opt.verbose()) {
                _opt.display.displayTable(table);
                _opt.display.out() << std::endl;
                break;
            }
            ts::TDT tdt(_opt.duck, table);
            if (!tdt.isValid()) {
                break;
            }
            std::cout << "* TDT UTC time: " << tdt.utc_time << std::endl;
            break;
        }

        case ts::TID_TOT: {
            if (_opt.no_tot) {
                break;
            }
            _tot_ok = !_opt.all;
            if (_opt.verbose()) {
                _opt.display.displayTable(table);
                _opt.display.out() << std::endl;
                break;
            }
            ts::TOT tot(_opt.duck, table);
            if (!tot.isValid()) {
                break;
            }
            std::cout << "* TOT UTC time: " << tot.utc_time << std::endl;
            for (const auto& region : tot.regions) {
                std::cout << "  Country: " << region.country
                          << ", region: " << region.region_id
                          << std::endl
                          << "  Local time:   " << tot.localTime(region)
                          << ", local time offset: "
                          << ts::TOT::timeOffsetFormat(region.time_offset)
                          << std::endl
                          << "  Next change:  " << region.next_change
                          << ", next time offset:  "
                          << ts::TOT::timeOffsetFormat(region.next_time_offset)
                          << std::endl;
            }
            break;
        }

        default: {
            if (_opt.verbose()) {
                const ts::TID tid = table.tableId();
                const ts::PID pid = table.sourcePID();
                std::cout << ts::UString::Format(u"* Got unexpected %s, TID %d (0x%X) on PID %d (0x%X)", {ts::names::TID(_opt.duck, tid), tid, tid, pid, pid}) << std::endl;
            }
        }
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Decode command line options.
    Options opt(argc, argv);

    // Configure the demux.
    TableHandler handler(opt);
    ts::SectionDemux demux(opt.duck, &handler);
    demux.addPID(ts::PID_TDT);  // also equal PID_TOT

    // Open the TS file.
    ts::TSFile file;
    if (!file.openRead(opt.infile, 1, 0, opt, opt.format)) {
        return EXIT_FAILURE;
    }

    // Read all packets in the file until the date is found.
    ts::TSPacket pkt;
    while (!handler.completed() && file.readPackets(&pkt, nullptr, 1, opt) > 0 ) {
        demux.feedPacket(pkt);
    }
    file.close(opt);

    return EXIT_SUCCESS;
}
