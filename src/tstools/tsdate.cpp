//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
#include "tsSectionDemux.h"
#include "tsStringUtils.h"
#include "tsNames.h"
#include "tsTDT.h"
#include "tsTOT.h"
#include "tsFormat.h"

using namespace ts;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public Args
{
    Options (int argc, char *argv[]);

    bool        no_tdt;      // Do not try to get a TDT
    bool        no_tot;      // Do not try to get a TOT
    bool        all;         // Report all tables, not only the first one.
    bool        verbose;     // Verbose output
    std::string infile;      // Input file name
};

Options::Options (int argc, char *argv[]) :
    Args ("MPEG Transport Stream Time (TDT/TOT) Extraction Utility.", "[options] [filename]")
{
    option ("",         0, Args::STRING, 0, 1);
    option ("all",     'a');
    option ("notdt",    0);
    option ("notot",    0);
    option ("verbose", 'v');

    setHelp ("Input file:\n"
             "\n"
             "  MPEG capture file (standard input if omitted).\n"
             "\n"
             "Options:\n"
             "\n"
             "  -a\n"
             "  --all\n"
             "      Report all TDT/TOT tables (default: report only the first table\n"
             "      of each type).\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  --notdt\n"
             "      Ignore Time & Date Table (TDT).\n"
             "\n"
             "  --notot\n"
             "      Ignore Time Offset Table (TOT).\n"
             "\n"
             "  -v\n"
             "  --verbose\n"
             "      Produce verbose output.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");

    analyze (argc, argv);

    infile = value ("");
    all = present ("all");
    verbose = present ("verbose");
    no_tdt = present ("notdt");
    no_tot = present ("notot");
}


//----------------------------------------------------------------------------
//  Table handler: receives TOT and TDT
//----------------------------------------------------------------------------

class TableHandler: public TableHandlerInterface
{
private:
    const Options& _opt;
    bool _tdt_ok;  // Finished TDT processing
    bool _tot_ok;  // Finished TOT processing

public:
    // Constructor
    TableHandler (const Options& opt) :
        _opt (opt),
        _tdt_ok (opt.no_tdt),
        _tot_ok (opt.no_tot)
    {
    }

    // Return true when analysis is complete
    bool completed() const
    {
        return _tdt_ok && _tot_ok;
    }

    // This hook is invoked when a complete table is available.
    virtual void handleTable (SectionDemux&, const BinaryTable&);
};


//----------------------------------------------------------------------------
// This hook is invoked when a complete table is available.
//----------------------------------------------------------------------------

void TableHandler::handleTable (SectionDemux&, const BinaryTable& table)
{
    switch (table.tableId()) {

        case TID_TDT: {
            if (_opt.no_tdt) {
                break;
            }
            _tdt_ok = !_opt.all;
            if (_opt.verbose) {
                std::cout << table << std::endl;
                break;
            }
            TDT tdt (table);
            if (!tdt.isValid()) {
                break;
            }
            std::cout << "* TDT UTC time: " << tdt.utc_time << std::endl;
            break;
        }

        case TID_TOT: {
            if (_opt.no_tot) {
                break;
            }
            _tot_ok = !_opt.all;
            if (_opt.verbose) {
                std::cout << table << std::endl;
                break;
            }
            TOT tot (table);
            if (!tot.isValid()) {
                break;
            }
            std::cout << "* TOT UTC time: " << tot.utc_time << std::endl;
            for (TOT::RegionVector::const_iterator it = tot.regions.begin(); it != tot.regions.end(); ++it) {
                std::cout << "  Country: " << Printable (it->country)
                          << ", region: " << it->region_id
                          << std::endl
                          << "  Local time:   " << tot.localTime (*it)
                          << ", local time offset: "
                          << TOT::timeOffsetFormat (it->time_offset)
                          << std::endl
                          << "  Next change:  " << it->next_change
                          << ", next time offset:  "
                          << TOT::timeOffsetFormat (it->next_time_offset)
                          << std::endl;
            }
            break;
        }

        default: {
            if (_opt.verbose) {
                const ts::TID tid = table.tableId();
                const ts::PID pid = table.sourcePID();
                std::cout << "* Got unexpected " << names::TID (tid)
                          << Format (", TID %d (0x%02X) on PID %d (0x%04X)",
                                     int (tid), int (tid), int (pid), int (pid))
                          << std::endl;
                }
        }
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
    Options opt (argc, argv);
    TableHandler handler (opt);
    SectionDemux demux (&handler);
    InputRedirector input (opt.infile, opt);
    TSPacket pkt;

    demux.addPID (PID_TDT);  // also equal PID_TOT

    while (!handler.completed() && pkt.read (std::cin, true, opt)) {
        demux.feedPacket (pkt);
    }

    return EXIT_SUCCESS;
}
