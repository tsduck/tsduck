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
//  Display date & time information (TDT & TOT) from a transport stream
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsInputRedirector.h"
#include "tsTablesDisplay.h"
#include "tsSectionDemux.h"
#include "tsNames.h"
#include "tsTDT.h"
#include "tsTOT.h"
#include "tsVersionInfo.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

class Options: public ts::Args
{
public:
    Options(int argc, char *argv[]);

    bool        no_tdt;   // Do not try to get a TDT
    bool        no_tot;   // Do not try to get a TOT
    bool        all;      // Report all tables, not only the first one.
    ts::UString infile;   // Input file name
};

Options::Options(int argc, char *argv[]) :
    ts::Args(u"MPEG Transport Stream Time (TDT/TOT) Extraction Utility.", u"[options] [filename]"),
    no_tdt(false),
    no_tot(false),
    all(false),
    infile()
{
    option(u"",         0, Args::STRING, 0, 1);
    option(u"all",     'a');
    option(u"notdt",    0);
    option(u"notot",    0);

    setHelp(u"Input file:\n"
            u"\n"
            u"  MPEG capture file (standard input if omitted).\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  -a\n"
            u"  --all\n"
            u"      Report all TDT/TOT tables (default: report only the first table\n"
            u"      of each type).\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  --notdt\n"
            u"      Ignore Time & Date Table (TDT).\n"
            u"\n"
            u"  --notot\n"
            u"      Ignore Time Offset Table (TOT).\n"
            u"\n"
            u"  -v\n"
            u"  --verbose\n"
            u"      Produce verbose output.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");

    analyze(argc, argv);

    infile = value(u"");
    all = present(u"all");
    no_tdt = present(u"notdt");
    no_tot = present(u"notot");

    exitOnError();
}


//----------------------------------------------------------------------------
//  Table handler: receives TOT and TDT
//----------------------------------------------------------------------------

class TableHandler: public ts::TableHandlerInterface
{
private:
    Options&          _opt;
    ts::TablesDisplay _display;
    bool              _tdt_ok;  // Finished TDT processing
    bool              _tot_ok;  // Finished TOT processing

public:
    // Constructor
    TableHandler(Options& opt) :
        _opt(opt),
        _display(ts::TablesDisplayArgs(), _opt),
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
                _display.displayTable(table) << std::endl;
                break;
            }
            ts::TDT tdt(table);
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
                _display.displayTable(table) << std::endl;
                break;
            }
            ts::TOT tot(table);
            if (!tot.isValid()) {
                break;
            }
            std::cout << "* TOT UTC time: " << tot.utc_time << std::endl;
            for (ts::TOT::RegionVector::const_iterator it = tot.regions.begin(); it != tot.regions.end(); ++it) {
                std::cout << "  Country: " << it->country
                          << ", region: " << it->region_id
                          << std::endl
                          << "  Local time:   " << tot.localTime(*it)
                          << ", local time offset: "
                          << ts::TOT::timeOffsetFormat(it->time_offset)
                          << std::endl
                          << "  Next change:  " << it->next_change
                          << ", next time offset:  "
                          << ts::TOT::timeOffsetFormat(it->next_time_offset)
                          << std::endl;
            }
            break;
        }

        default: {
            if (_opt.verbose()) {
                const ts::TID tid = table.tableId();
                const ts::PID pid = table.sourcePID();
                std::cout << ts::UString::Format(u"* Got unexpected %s, TID %d (0x%X) on PID %d (0x%X)", {ts::names::TID(tid), tid, tid, pid, pid}) << std::endl;
            }
        }
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    TSDuckLibCheckVersion();
    Options opt(argc, argv);
    TableHandler handler(opt);
    ts::SectionDemux demux(&handler);
    ts::InputRedirector input(opt.infile, opt);
    ts::TSPacket pkt;

    demux.addPID(ts::PID_TDT);  // also equal PID_TOT

    while (!handler.completed() && pkt.read(std::cin, true, opt)) {
        demux.feedPacket(pkt);
    }

    return EXIT_SUCCESS;
}
