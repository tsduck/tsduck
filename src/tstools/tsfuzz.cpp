//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// Transport Stream file fuzzing utility
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsDuckContext.h"
#include "tsTSFuzzing.h"
#include "tsTSFile.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
// Command line options
//----------------------------------------------------------------------------

namespace ts {
    class FuzzOptions: public Args
    {
        TS_NOBUILD_NOCOPY(FuzzOptions);
    public:
        FuzzOptions(int argc, char *argv[]);
        virtual ~FuzzOptions() override;

        DuckContext           duck {this};  // TSDuck execution context.
        std::vector<fs::path> in_files {};  // Input file names.
        fs::path              out_file {};  // Output file name or directory.
        bool                  out_dir {};   // Output name is a directory.
        ts::TSFuzzingArgs     fuzz {};      // Fuzzing options.
    };
}

ts::FuzzOptions::FuzzOptions(int argc, char *argv[]) :
    Args(u"Introduce random errors in a transport stream file", u"[options] filename ...")
{
    fuzz.defineArgs(*this);

    option(u"", 0, FILENAME, 0, UNLIMITED_COUNT);
    help(u"",
         u"MPEG transport stream input files to corrupt. "
         u"If more than one file is specified, the output name shall specify a directory.");

    option(u"output", 'o', FILENAME, 1, 1);
    help(u"output",
         u"Output file or directory. "
         u"This is a mandatory parameter, there is no default. "
         u"If more than one input file is specified, the output name shall specify a directory.");

    analyze(argc, argv);

    fuzz.loadArgs(duck, *this);
    getPathValues(in_files, u"");
    getPathValue(out_file, u"output");
    out_dir = fs::is_directory(out_file);

    if (in_files.size() > 1 && !out_dir) {
        error(u"the output name must be a directory when more than one input file is specified");
    }
    exitOnError();
}

ts::FuzzOptions::~FuzzOptions()
{
}


//----------------------------------------------------------------------------
// Program entry point.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    ts::FuzzOptions opt(argc, argv);
    ts::TSFuzzing fuzzer(opt.duck);
    bool success = true;

    // Process input files one by one.
    for (const auto& infile_name : opt.in_files) {
        opt.verbose(u"Fuzzing %s", {infile_name});

        // Open input file.
        ts::TSFile infile;
        if (!infile.openRead(infile_name, 0, opt)) {
            success = false;
            continue;
        }

        // Output file name.
        fs::path outfile_name(opt.out_file);
        if (opt.out_dir) {
            // Output name is a directory.
            outfile_name /= infile_name.filename();
        }

        // Create output file.
        ts::TSFile outfile;
        if (!outfile.open(outfile_name, ts::TSFile::WRITE, opt)) {
            success = false;
            continue;
        }

        // Reinitialize the fuzzer.
        if (!fuzzer.start(opt.fuzz)) {
            success = false;
            break; // won't work on other files either
        }

        // Process all packets.
        ts::TSPacketVector pkts(1000);
        size_t count = 0;
        while ((count = infile.readPackets(pkts.data(), nullptr, pkts.size(), opt)) > 0) {
            for (size_t i = 0; i < count; ++i) {
                if (!fuzzer.processPacket(pkts[i])) {
                    success = false;
                    break;
                }
            }
            if (!outfile.writePackets(pkts.data(), nullptr, count, opt)) {
                success = false;
                break;
            }
        }
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
