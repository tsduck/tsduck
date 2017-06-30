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
//  Evaluate the bitrate of a transport stream based on PCR values.
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsInputRedirector.h"
#include "tsDecimal.h"
#include "tsHexa.h"
#include "tsPCRAnalyzer.h"
#include "tsFormat.h"
TSDUCK_SOURCE;

using namespace ts;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public Args
{
    Options (int argc, char *argv[]);

    uint32_t    min_pcr;     // Min # of PCR per PID
    uint16_t    min_pid;     // Min # of PID
    std::string pcr_name;    // Time stamp type name
    bool        use_dts;     // Use DTS instead of PCR
    bool        all;         // All packets analysis
    bool        full;        // Full analysis
    bool        value_only;  // Output value only
    std::string infile;      // Input file name
};

Options::Options (int argc, char *argv[]) :
    Args("MPEG Transport Stream Bitrate Evaluation Utility.", "[options] [filename]"),
    min_pcr(0),
    min_pid(0),
    pcr_name(),
    use_dts(false),
    all(false),
    full(false),
    value_only(false),
    infile()
{
    option ("",            0, Args::STRING, 0, 1);
    option ("all",        'a');
    option ("dts",        'd');
    option ("full",       'f');
    option ("min-pcr",     0, Args::POSITIVE);
    option ("min-pid",     0, Args::INTEGER, 0, 1, 1, PID_MAX);
    option ("value-only", 'v');

    setHelp ("Input file:\n"
             "\n"
             "  MPEG capture file (standard input if omitted).\n"
             "\n"
             "Options:\n"
             "\n"
             "  -a\n"
             "  --all\n"
             "      Analyze all packets in the input file. By default, stop analysis when\n"
             "      enough PCR information has been collected.\n"
             "\n"
             "  -d\n"
             "  --dts\n"
             "      Use DTS (Decoding Time Stamps) from video PID's instead of PCR\n"
             "      (Program Clock Reference) from the transport layer.\n"
             "\n"
             "  -f\n"
             "  --full\n"
             "      Full analysis. The file is entirely analyzed (as with --all) and the\n"
             "      final report includes a complete per PID bitrate analysis.\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  --min-pcr value\n"
             "      Stop analysis when that number of PCR are read from the required\n"
             "      minimum number of PID (default: 64).\n"
             "\n"
             "  --min-pid value\n"
             "      Minimum number of PID to get PCR from (default: 1).\n"
             "\n"
             "  -v\n"
             "  --value-only\n"
             "      Display only the bitrate value, in bits/seconds, based on\n"
             "      188-byte packets. Useful to reuse the value in command lines.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");

    analyze (argc, argv);

    infile = value ("");
    full = present ("full");
    all = full || present ("all");
    value_only = present ("value-only");
    min_pcr = intValue<uint32_t> ("min-pcr", 64);
    min_pid = intValue<uint16_t> ("min-pid", 1);
    use_dts = present ("dts");
    pcr_name = use_dts ? "DTS" : "PCR";
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
    Options opt (argc, argv);
    PCRAnalyzer zer (opt.min_pid, opt.min_pcr);
    InputRedirector input (opt.infile, opt);
    TSPacket pkt;

    // Reset analyzer for DTS with --dts
    if (opt.use_dts) {
        zer.resetAndUseDTS (opt.min_pid, opt.min_pcr);
    }

    // Read all packets in the file and pass them to the PCR analyzer.
    while (pkt.read (std::cin, true, opt) && (!zer.feedPacket (pkt) || opt.all)) {}

    // Display results.
    PCRAnalyzer::Status status;
    zer.getStatus (status);

    if (!status.bitrate_valid) {
        opt.error ("cannot compute transport bitrate, insufficient " + opt.pcr_name);
        if (!opt.full) {
            return EXIT_FAILURE;
        }
    }

    if (opt.value_only) {
        std::cout << status.bitrate_188 << std::endl;
        return EXIT_SUCCESS;
    }

    if (opt.full) {
        std::cout << std::endl
                  << "Transport Stream" << std::endl
                  << "----------------" << std::endl;
        if (!opt.infile.empty()) {
            std::cout << "File           : " << opt.infile << std::endl;
        }
        std::cout << "TS packets     : " << Decimal (status.packet_count) << std::endl
                  << opt.pcr_name << "            : " << Decimal (status.pcr_count) << std::endl
                  << "PIDs with " << opt.pcr_name << "  : " << Decimal (status.pcr_pids) << std::endl;
    }

    std::cout << "TS bitrate" << (opt.full ? "     " : "") << ": "
              << Decimal (status.bitrate_188) << " b/s (188-byte), "
              << Decimal (status.bitrate_204) << " b/s (204-byte)"
              << std::endl;

    if (opt.full) {
        std::cout << std::endl
                  << "PID              TS Packets  Bitrate (188-byte)  Bitrate (204-byte)" << std::endl
                  << "-------------  ------------  ------------------  ------------------" << std::endl;
        for (ts::PID pid = 0; pid < PID_MAX; pid++) {
            PacketCounter pcount = zer.packetCount (pid);
            if (pcount > 0) {
                std::cout << Format ("%4d (0x%04X)  ", int (pid), int (pid))
                          << Decimal (pcount, 12) << "  "
                          << Decimal (zer.bitrate188 (pid), 14) << " b/s  "
                          << Decimal (zer.bitrate204 (pid), 14) << " b/s"
                          << std::endl;
            }
        }
        std::cout << std::endl;
    }

    return EXIT_SUCCESS;
}
