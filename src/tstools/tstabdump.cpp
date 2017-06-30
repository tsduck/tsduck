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
//  Dump PSI/SI tables, as saved by tstables.
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsTablesDisplay.h"
#include "tsCASFamily.h"
#include "tsSection.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public ts::Args
{
    Options(int argc, char *argv[]);

    bool                  verbose;   // Verbose output
    ts::StringVector      infiles;   // Input file names
    ts::TablesDisplayArgs display;   // Options about displaying tables
};

Options::Options(int argc, char *argv[]) :
    ts::Args("Dump PSI/SI tables, as saved by tstables.", "[options] [filename ...]"),
    verbose(false),
    infiles(),
    display()
{
    // Warning, the following short options are already defined in TablesDisplayArgs:
    // 'c', 'r'
    option("", 0, ts::Args::STRING);
    option("verbose", 'v');
    display.defineOptions(*this);

    setHelp("Input file:\n"
            "\n"
            "  MPEG capture file (standard input if omitted).\n"
            "\n"
            "Options:\n"
            "\n"
            "  --help\n"
            "      Display this help text.\n"
            "\n"
            "  -v\n"
            "  --verbose\n"
            "      Produce verbose output.\n"
            "\n"
            "  --version\n"
            "      Display the version number.\n");
    display.addHelp(*this);

    analyze(argc, argv);
    display.load(*this);

    getValues(infiles, "");
    verbose = present("verbose");
}


//----------------------------------------------------------------------------
//  Dump routine. Return true on success.
//----------------------------------------------------------------------------

bool DumpFile(Options& opt, const std::string& file_name)
{
    // Report file name in case of multiple files
    if (opt.verbose && opt.infiles.size() > 1) {
        std::cout << "* File: " << file_name << std::endl << std::endl;
    }

    // Load all sections
    ts::SectionPtrVector sections;
    bool ok;

    if (opt.infiles.size() == 0) {
        // no input file specified, use standard input
        SetBinaryModeStdin(opt);
        ok = ts::Section::LoadFile(sections, std::cin, ts::CRC32::IGNORE, opt);
    }
    else {
        ok = ts::Section::LoadFile(sections, file_name, ts::CRC32::IGNORE, opt);
    }

    if (ok) {
        // Display all sections.
        ts::TablesDisplay display(opt.display, opt);
        for (ts::SectionPtrVector::const_iterator it = sections.begin(); it != sections.end(); ++it) {
            display.displaySection(std::cout, **it);
        }
    }

    return ok;
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    Options opt(argc, argv);
    bool ok = true;

    std::cout << std::endl;

    if (opt.infiles.size() == 0) {
        ok = DumpFile(opt, "");
    }
    else {
        for (ts::StringVector::const_iterator it = opt.infiles.begin(); it != opt.infiles.end(); ++it) {
            ok = DumpFile(opt, *it) && ok;
        }
    }

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
