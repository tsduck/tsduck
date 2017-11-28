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
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------


struct Options: public ts::Args
{
    Options(int argc, char *argv[]);

    bool                   continuous; // Continuous packetization
    ts::CyclingPacketizer::StuffingPolicy stuffing_policy;
    ts::CRC32::Validation  crc_op;     // Validate/recompute CRC32
    ts::PID                pid;        // Target PID
    ts::BitRate            bitrate;    // Target PID bitrate
    std::string            outfile;    // Output file
    ts::FileNameRateList   infiles;    // Input file names and repetition rates
    bool                   verbose;    // Verbose mode
    bool                   debug;      // Debug mode
};

Options::Options(int argc, char *argv[]) :
    ts::Args("Packetize PSI/SI sections in a transport stream PID.", "[options] [input-file[=rate] ...]"),
    continuous(false),
    stuffing_policy(ts::CyclingPacketizer::NEVER),
    crc_op(ts::CRC32::COMPUTE),
    pid(ts::PID_NULL),
    bitrate(0),
    outfile(),
    infiles(),
    verbose(false),
    debug(false)
{
    option(u"",            0,  Args::STRING);
    option(u"bitrate",    'b', Args::UNSIGNED);
    option(u"continuous", 'c');
    option(u"debug",      'd');
    option(u"force-crc",  'f');
    option(u"output",     'o', Args::STRING);
    option(u"pid",        'p', Args::PIDVAL, 1, 1);
    option(u"stuffing",   's');
    option(u"verbose",    'v');

    setHelp(u"Input files:\n"
            u"\n"
            u"  Binary files containing sections (standard input if omitted).\n"
            u"  If different repetition rates are required for different files,\n"
            u"  a parameter can be \"filename=value\" where value is the\n"
            u"  repetition rate in milliseconds for all sections in that file.\n"
            u"  For repetition rates to be effective, the bitrate of the target\n"
            u"  PID must be specified, see option -b or --bitrate.\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  -b value\n"
            u"  --bitrate value\n"
            u"      Specifies the bitrate (in bits/second) of the target PID. This\n"
            u"      information is used to schedule sections in the output list of\n"
            u"      packets when specific bitrates are specified for sections.\n"
            u"\n"
            u"  -c\n"
            u"  --continuous\n"
            u"      Continuous packetization. By default, generate one cycle of sections.\n"
            u"\n"
            u"  -f\n"
            u"  --force-crc\n"
            u"      Force recomputation of CRC32 in long sections. Ignore the CRC32\n"
            u"      values in the input files.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -o file-name\n"
            u"  --output file-name\n"
            u"      Output file name for TS packets. By default, use standard output.\n"
            u"\n"
            u"  -p value\n"
            u"  --pid value\n"
            u"      PID of the output TS packets. This is a required parameter, there is\n"
            u"      no default value.\n"
            u"\n"
            u"  -s\n"
            u"  --stuffing\n"
            u"      Insert stuffing at end of each section, up to the next TS packet\n"
            u"      boundary. By default, sections are packed and start in the middle\n"
            u"      of a TS packet, after the previous section. Note, however, that\n"
            u"      section headers are never scattered over a packet boundary.\n"
            u"\n"
            u"  -v\n"
            u"  --verbose\n"
            u"      Display verbose information.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");

    analyze(argc, argv);

    continuous = present(u"continuous");
    if (present(u"stuffing")) {
        stuffing_policy = ts::CyclingPacketizer::ALWAYS;
    }
    else if (continuous) {
        stuffing_policy = ts::CyclingPacketizer::NEVER;
    }
    else {
        stuffing_policy = ts::CyclingPacketizer::AT_END;
    }
    crc_op = present(u"force-crc") ? ts::CRC32::COMPUTE : ts::CRC32::CHECK;
    pid = intValue<ts::PID>(u"pid", ts::PID_NULL);
    bitrate = intValue<ts::BitRate>(u"bitrate");
    outfile = value(u"output");
    infiles.getArgs(*this);
    debug = present(u"debug");
    verbose = debug || present(u"verbose");

    // If any non-zero repetition rate is specified, make sure that a bitrate
    // is specified.
    for (ts::FileNameRateList::const_iterator it = infiles.begin(); it != infiles.end(); ++it) {
        if (it->repetition != 0 && bitrate == 0) {
            error(u"the PID bitrate must be specified when repetition rates are used");
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
    ts::OutputRedirector output(opt.outfile, opt);
    ts::CyclingPacketizer pzer(opt.pid, opt.stuffing_policy, opt.bitrate);
    ts::SectionPtrVector sections;

    // Load sections

    if (opt.infiles.size() == 0) {
        // Read sections from standard input
        SetBinaryModeStdin(opt);
        if (!ts::Section::LoadFile(sections, std::cin, opt.crc_op, opt)) {
            return EXIT_FAILURE;
        }
        pzer.addSections(sections);
        if (opt.verbose) {
            std::cerr << "* Loaded " << sections.size() << " sections from standard input" << std::endl;
        }
    }
    else {
        for (ts::FileNameRateList::const_iterator it = opt.infiles.begin(); it != opt.infiles.end(); ++it) {
            if (!ts::Section::LoadFile(sections, it->file_name, opt.crc_op, opt)) {
                return EXIT_FAILURE;
            }
            pzer.addSections(sections, it->repetition);
            if (opt.verbose) {
                std::cerr << "* Loaded " << sections.size() << " sections from " << it->file_name;
                if (it->repetition > 0) {
                    std::cerr << ", repetition rate: " << ts::Decimal(it->repetition) << " ms";
                }
                std::cerr << std::endl;
            }
        }
    }

    if (opt.debug) {
        std::cerr << "* Before packetization:" << std::endl;
        pzer.display(std::cerr);
    }

    // Generate packets

    ts::TSPacket pkt;
    ts::PacketCounter count = 0;

    do {
        pzer.getNextPacket(pkt);
        pkt.write(std::cout, opt);
        count++;
    } while (opt.valid() && (opt.continuous || !pzer.atCycleBoundary()));


    if (opt.verbose) {
        std::cerr << "* Generated " << ts::Decimal(count) << " TS packets" << std::endl;
    }
    if (opt.debug) {
        std::cerr << "* After packetization:" << std::endl;
        pzer.display(std::cerr);
    }

    return opt.valid() ? EXIT_SUCCESS : EXIT_FAILURE;
}
