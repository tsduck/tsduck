//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  DVB-NIP analysis tool for pcap and pcap-ng files.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsPagerArgs.h"
#include "tsReportFile.h"
#include "tsNIPAnalyzer.h"
#include "tsIPPacket.h"
#include "tsIPProtocols.h"
#include "tsPcapFilter.h"
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

        ts::DuckContext     duck {this};
        ts::UString         input_file {};
        ts::PcapFilter      file {};
        ts::PagerArgs       pager {true, true};
        ts::NIPAnalyzerArgs nip {};
    };
}

// Get command line options.
Options::Options(int argc, char *argv[]) :
    ts::Args(u"Analyze a DVB-NIP stream from a pcap or pcap-ng file", u"[options] [input-file]")
{
    file.defineArgs(*this);
    pager.defineArgs(*this);
    nip.defineArgs(*this);

    option(u"", 0, FILENAME, 0, 1);
    help(u"", u"file-name",
         u"Input file in pcap or pcap-ng format, typically as saved by Wireshark. "
         u"Use the standard input if no file name is specified.");

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    file.loadArgs(*this);
    pager.loadArgs(*this);
    nip.loadArgs(duck, *this);
    getValue(input_file, u"");
    exitOnError();
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
    report.setReportPrefix(u"\n* ");
    opt.duck.setReport(&report);

    // Initialize a DVB-NIP analyzer.
    ts::NIPAnalyzer analyzer(opt.duck);
    analyzer.reset(opt.nip);

    // Read all IP packets from the file.
    ts::IPPacket ip;
    ts::VLANIdStack vlans;
    cn::microseconds timestamp;
    while (opt.file.readIP(ip, vlans, timestamp, opt)) {
        analyzer.feedPacket(ip);
    }
    opt.file.close();

    return EXIT_SUCCESS;
}
