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
//  Packetize PSI/SI tables in a transport stream PID.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsSectionFileArgs.h"
#include "tsTSPacket.h"
#include "tsFileNameRateList.h"
#include "tsOutputRedirector.h"
#include "tsCyclingPacketizer.h"
#include "tsSysUtils.h"
TS_MAIN(MainCode);

// To avoid long prefixes
typedef ts::CyclingPacketizer::StuffingPolicy StuffPolicy;
typedef ts::SectionFile::FileType FType;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace {
    class Options: public ts::Args
    {
        TS_NOBUILD_NOCOPY(Options);
    public:
        Options(int argc, char *argv[]);

        ts::DuckContext       duck;
        bool                  continuous;    // Continuous packetization
        StuffPolicy           stuffing_policy;
        ts::CRC32::Validation crc_op;        // Validate/recompute CRC32
        ts::PID               pid;           // Target PID
        ts::BitRate           bitrate;       // Target PID bitrate
        ts::UString           outfile;       // Output file
        ts::FileNameRateList  infiles;       // Input file names and repetition rates
        FType                 inType;        // Input files type
        ts::SectionFileArgs   sections_opt;  // Section file options
    };
}

Options::Options(int argc, char *argv[]) :
    Args(u"Packetize PSI/SI sections in a transport stream PID", u"[options] [input-file[=rate] ...]"),
    duck(this),
    continuous(false),
    stuffing_policy(StuffPolicy::NEVER),
    crc_op(ts::CRC32::COMPUTE),
    pid(ts::PID_NULL),
    bitrate(0),
    outfile(),
    infiles(),
    inType(FType::UNSPECIFIED),
    sections_opt()
{
    duck.defineArgsForCharset(*this);
    sections_opt.defineArgs(*this);

    option(u"", 0, FILENAME);
    help(u"", u"filename[=rate]",
         u"Input binary, XML or JSON files containing one or more sections or tables. "
         u"By default, files ending in .bin, .xml or .json are automatically recognized. "
         u"For other file names, explicitly specify --binary, --xml or --json. "
         u"If the file name is omitted, the standard input is used (binary by default, specify --xml or --json otherwise).\n\n"
         u"The reference source format is XML. JSON files are first translated to XML using the "
         u"\"automated XML-to-JSON conversion\" rules of TSDuck and then compiled to binary.\n\n"
         u"If different repetition rates are required for different files, a parameter can be "
         u"\"filename=value\" where value is the repetition rate in milliseconds for all sections in that file. "
         u"For repetition rates to be effective, the bitrate of the target PID must be specified, see option -b or --bitrate.");

    option(u"binary", 0);
    help(u"binary", u"Specify that all input files are binary, regardless of their file name.");

    option<ts::BitRate>(u"bitrate", 'b');
    help(u"bitrate",
         u"Specifies the bitrate (in bits/second) of the target PID. This "
         u"information is used to schedule sections in the output list of "
         u"packets when specific bitrates are specified for sections.");

    option(u"continuous", 'c');
    help(u"continuous", u"Continuous packetization. By default, generate one cycle of sections.");

    option(u"force-crc", 'f');
    help(u"force-crc", u"Force recomputation of CRC32 in long sections. Ignore the CRC32 values in the input files.");

    option(u"json", 'j');
    help(u"json", u"Specify that all input files are JSON, regardless of their file name.");

    option(u"output", 'o', FILENAME);
    help(u"output", u"Output file name for TS packets. By default, use standard output.");

    option(u"pid", 'p', PIDVAL, 1, 1);
    help(u"pid", u"PID of the output TS packets. This is a required parameter, there is no default value.");

    option(u"stuffing", 's');
    help(u"stuffing",
         u"Insert stuffing at end of each section, up to the next TS packet "
         u"boundary. By default, sections are packed and start in the middle "
         u"of a TS packet, after the previous section. Note, however, that "
         u"section headers are never scattered over a packet boundary.");

    option(u"xml", 'x');
    help(u"xml", u"Specify that all input files are XML, regardless of their file name.");

    analyze(argc, argv);
    duck.loadArgs(*this);
    sections_opt.loadArgs(duck, *this);

    continuous = present(u"continuous");
    if (present(u"stuffing")) {
        stuffing_policy = StuffPolicy::ALWAYS;
    }
    else if (continuous) {
        stuffing_policy = StuffPolicy::NEVER;
    }
    else {
        stuffing_policy = StuffPolicy::AT_END;
    }
    crc_op = present(u"force-crc") ? ts::CRC32::COMPUTE : ts::CRC32::CHECK;
    getIntValue(pid, u"pid", ts::PID_NULL);
    getValue(bitrate, u"bitrate");
    getValue(outfile, u"output");
    infiles.getArgs(*this);
    if (present(u"xml")) {
        inType = FType::XML;
    }
    else if (present(u"json")) {
        inType = FType::JSON;
    }
    else if (present(u"binary")) {
        inType = FType::BINARY;
    }

    // If any non-zero repetition rate is specified, make sure that a bitrate is specified.
    for (const auto& it : infiles) {
        if (it.repetition != 0 && bitrate == 0) {
            error(u"the PID bitrate must be specified when repetition rates are used");
            break;
        }
    }

    exitOnError();
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    Options opt(argc, argv);
    ts::OutputRedirector output(opt.outfile, opt);
    ts::CyclingPacketizer pzer(opt.duck, opt.pid, opt.stuffing_policy, opt.bitrate);
    ts::SectionFile file(opt.duck);
    file.setCRCValidation(opt.crc_op);

    // Load sections
    if (opt.infiles.size() == 0) {
        // Read sections from standard input.
        if (opt.inType != FType::XML && opt.inType != FType::JSON) {
            // Default type for standard input is binary.
            SetBinaryModeStdin(opt);
            opt.inType = FType::BINARY;
        }
        if (!file.load(std::cin, opt.inType) || !opt.sections_opt.processSectionFile(file, opt)) {
            return EXIT_FAILURE;
        }
        pzer.addSections(file.sections());
        if (opt.verbose()) {
            std::cerr << "* Loaded " << file.sections().size() << " sections from standard input" << std::endl;
        }
    }
    else {
        for (const auto& it : opt.infiles) {
            file.clear();
            if (!file.load(it.file_name, opt.inType) || !opt.sections_opt.processSectionFile(file, opt)) {
                return EXIT_FAILURE;
            }
            pzer.addSections(file.sections(), it.repetition);
            if (opt.verbose()) {
                std::cerr << "* Loaded " << file.sections().size() << " sections from " << it.display_name;
                if (it.repetition > 0) {
                    std::cerr << ", repetition rate: " << ts::UString::Decimal(it.repetition) << " ms";
                }
                std::cerr << std::endl;
            }
        }
    }

    if (opt.debug()) {
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

    if (opt.verbose()) {
        std::cerr << "* Generated " << ts::UString::Decimal(count) << " TS packets" << std::endl;
    }
    if (opt.debug()) {
        std::cerr << "* After packetization:" << std::endl;
        pzer.display(std::cerr);
    }

    return opt.valid() ? EXIT_SUCCESS : EXIT_FAILURE;
}
