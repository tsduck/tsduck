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
//  Packetize PSI/SI tables in a transport stream PID.
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsFileNameRate.h"
#include "tsOutputRedirector.h"
#include "tsCyclingPacketizer.h"
#include "tsDecimal.h"
#include "tsSysUtils.h"

using namespace ts;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------


struct Options: public Args
{
    Options (int argc, char *argv[]);

    bool               continuous; // Continuous packetization
    CyclingPacketizer::StuffingPolicy stuffing_policy;
    CRC32::Validation  crc_op;     // Validate/recompute CRC32
    ts::PID          pid;        // Target PID
    BitRate            bitrate;    // Target PID bitrate
    std::string        outfile;    // Output file
    FileNameRateVector infiles;    // Input file names and repetition rates
    bool               verbose;    // Verbose mode
    bool               debug;      // Debug mode
};

Options::Options (int argc, char *argv[]) :
    Args ("Packetize PSI/SI sections in a transport stream PID.", "[options] [input-file[=rate] ...]"),
    infiles ()
{
    option ("",            0,  Args::STRING);
    option ("bitrate",    'b', Args::UNSIGNED);
    option ("continuous", 'c');
    option ("debug",      'd');
    option ("force-crc",  'f');
    option ("output",     'o', Args::STRING);
    option ("pid",        'p', Args::PIDVAL, 1, 1);
    option ("stuffing",   's');
    option ("verbose",    'v');

    setHelp ("Input files:\n"
             "\n"
             "  Binary files containing sections (standard input if omitted).\n"
             "  If different repetition rates are required for different files,\n"
             "  a parameter can be \"filename=value\" where value is the\n"
             "  repetition rate in milliseconds for all sections in that file.\n"
             "  For repetition rates to be effective, the bitrate of the target\n"
             "  PID must be specified, see option -b or --bitrate.\n"
             "\n"
             "Options:\n"
             "\n"
             "  -b value\n"
             "  --bitrate value\n"
             "      Specifies the bitrate (in bits/second) of the target PID. This\n"
             "      information is used to schedule sections in the output list of\n"
             "      packets when specific bitrates are specified for sections.\n"
             "\n"
             "  -c\n"
             "  --continuous\n"
             "      Continuous packetization. By default, generate one cycle of sections.\n"
             "\n"
             "  -f\n"
             "  --force-crc\n"
             "      Force recomputation of CRC32 in long sections. Ignore the CRC32\n"
             "      values in the input files.\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -o file-name\n"
             "  --output file-name\n"
             "      Output file name for TS packets. By default, use standard output.\n"
             "\n"
             "  -p value\n"
             "  --pid value\n"
             "      PID of the output TS packets. This is a required parameter, there is\n"
             "      no default value.\n"
             "\n"
             "  -s\n"
             "  --stuffing\n"
             "      Insert stuffing at end of each section, up to the next TS packet\n"
             "      boundary. By default, sections are packed and start in the middle\n"
             "      of a TS packet, after the previous section. Note, however, that\n"
             "      section headers are never scattered over a packet boundary.\n"
             "\n"
             "  -v\n"
             "  --verbose\n"
             "      Display verbose information.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");

    analyze (argc, argv);

    continuous = present ("continuous");
    if (present ("stuffing")) {
        stuffing_policy = CyclingPacketizer::ALWAYS;
    }
    else if (continuous) {
        stuffing_policy = CyclingPacketizer::NEVER;
    }
    else {
        stuffing_policy = CyclingPacketizer::AT_END;
    }
    crc_op = present ("force-crc") ? CRC32::COMPUTE : CRC32::CHECK;
    pid = intValue<ts::PID> ("pid", ts::PID (PID_NULL));
    bitrate = intValue<BitRate> ("bitrate");
    outfile = value ("output");
    GetFileNameRates (infiles, *this);
    debug = present ("debug");
    verbose = debug || present ("verbose");

    // If any non-zero repetition rate is specified, make sure that a bitrate
    // is specified.
    for (FileNameRateVector::const_iterator it = infiles.begin(); it != infiles.end(); ++it) {
        if (it->repetition != 0 && bitrate == 0) {
            error ("the PID bitrate must be specified when repetition rates are used");
            break;
        }
    }

    exitOnError();
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
    Options opt (argc, argv);
    OutputRedirector output (opt.outfile, opt);
    CyclingPacketizer pzer (opt.pid, opt.stuffing_policy, opt.bitrate);
    SectionPtrVector sections;

    // Load sections

    if (opt.infiles.size() == 0) {
        // Read sections from standard input
        SetBinaryModeStdin (opt);
        if (!Section::LoadFile (sections, std::cin, opt.crc_op, opt)) {
            return EXIT_FAILURE;
        }
        pzer.addSections (sections);
        if (opt.verbose) {
            std::cerr << "* Loaded " << sections.size() << " sections from standard input" << std::endl;
        }
    }
    else {
        for (FileNameRateVector::const_iterator it = opt.infiles.begin(); it != opt.infiles.end(); ++it) {
            if (!Section::LoadFile (sections, it->file_name, opt.crc_op, opt)) {
                return EXIT_FAILURE;
            }
            pzer.addSections (sections, it->repetition);
            if (opt.verbose) {
                std::cerr << "* Loaded " << sections.size() << " sections from " << it->file_name;
                if (it->repetition > 0) {
                    std::cerr << ", repetition rate: " << Decimal (it->repetition) << " ms";
                }
                std::cerr << std::endl;
            }
        }
    }

    if (opt.debug) {
        std::cerr << "* Before packetization:" << std::endl;
        pzer.display (std::cerr);
    }

    // Generate packets

    TSPacket pkt;
    PacketCounter count = 0;

    do {
        pzer.getNextPacket (pkt);
        pkt.write (std::cout, opt);
        count++;
    } while (opt.valid() && (opt.continuous || !pzer.atCycleBoundary()));

    
    if (opt.verbose) {
        std::cerr << "* Generated " << Decimal (count) << " TS packets" << std::endl;
    }
    if (opt.debug) {
        std::cerr << "* After packetization:" << std::endl;
        pzer.display (std::cerr);
    }

    return opt.valid() ? EXIT_SUCCESS : EXIT_FAILURE;
}
