//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  FLUTE analysis tool for pcap and pcap-ng files.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsPagerArgs.h"
#include "tsReportFile.h"
#include "tsIPPacket.h"
#include "tsIPProtocols.h"
#include "tsPcapFilter.h"
#include "tsmcastFluteAnalyzer.h"
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

        ts::DuckContext duck {this};
        ts::UString     input_file {};
        ts::PcapFilter  file {};
        ts::PagerArgs   pager {true, true};
        ts::mcast::FluteAnalyzerArgs flute {};
    };
}

// Get command line options.
Options::Options(int argc, char *argv[]) :
    ts::Args(u"Analyze a FLUTE stream from a pcap or pcap-ng file", u"[options] [input-file]")
{
    file.defineArgs(*this);
    pager.defineArgs(*this);
    flute.defineArgs(*this);

    option(u"", 0, FILENAME, 0, 1);
    help(u"", u"file-name",
         u"Input file in pcap or pcap-ng format, typically as saved by Wireshark. "
         u"Use the standard input if no file name is specified.");

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    file.loadArgs(*this);
    pager.loadArgs(*this);
    flute.loadArgs(duck, *this);
    getValue(input_file, u"");
    exitOnError();

    // Don't page if there is nothing to do except a summary in a specified output file.
    if (flute.none(true) && !flute.output_file.empty() && flute.output_file != u"-") {
        pager.use_pager = false;
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
    if (!opt.file.open(opt.input_file, opt)) {
        return EXIT_FAILURE;
    }

    // Read UDP packets only.
    opt.file.setProtocolFilterUDP();

    // Setup an output pager if necessary.
    std::ostream& out(opt.pager.output(opt));
    ts::ReportFile<ts::ThreadSafety::None> report(out, opt.maxSeverity());
    report.setReportPrefix(u"* ");
    opt.duck.setReport(&report);

    // Initialize a Flute analyzer.
    ts::mcast::FluteAnalyzer analyzer(opt.duck);
    if (!analyzer.reset(opt.flute)) {
        return EXIT_FAILURE;
    }

    // Read all IP packets from the file.
    ts::IPPacket ip;
    ts::VLANIdStack vlans;
    cn::microseconds timestamp;
    while (opt.file.readIP(ip, vlans, timestamp, opt)) {
        analyzer.feedPacket(timestamp, ip);
    }
    opt.file.close();

    // Report final summary.
    if (opt.flute.summary) {
        analyzer.printSummary(out);
    }

    return EXIT_SUCCESS;
}
