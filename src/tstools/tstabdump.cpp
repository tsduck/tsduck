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
//  Dump PSI/SI tables, as saved by tstables.
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsSectionFile.h"
#include "tsTablesDisplay.h"
#include "tsCASFamily.h"
#include "tsSection.h"
#include "tsSysUtils.h"
#include "tsVersionInfo.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public ts::Args
{
    Options(int argc, char *argv[]);

    ts::UStringVector     infiles;   // Input file names
    ts::TablesDisplayArgs display;   // Options about displaying tables
};

Options::Options(int argc, char *argv[]) :
    ts::Args(u"Dump PSI/SI tables, as saved by tstables.", u"[options] [filename ...]"),
    infiles(),
    display()
{
    // Warning, the following short options are already defined in TablesDisplayArgs:
    // 'c', 'r'
    option(u"", 0, ts::Args::STRING);
    display.defineOptions(*this);

    setHelp(u"Input file:\n"
            u"\n"
            u"  MPEG capture file (standard input if omitted).\n"
            u"\n"
            u"Options:\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -v\n"
            u"  --verbose\n"
            u"      Produce verbose output.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
    display.addHelp(*this);

    analyze(argc, argv);

    getValues(infiles, u"");
    display.load(*this);

    exitOnError();
}


//----------------------------------------------------------------------------
//  Dump routine. Return true on success.
//----------------------------------------------------------------------------

bool DumpFile(Options& opt, const ts::UString& file_name)
{
    // Report file name in case of multiple files
    if (opt.verbose() && opt.infiles.size() > 1) {
        std::cout << "* File: " << file_name << std::endl << std::endl;
    }

    // Load all sections
    ts::SectionFile file;
    bool ok;

    if (file_name.empty()) {
        // no input file specified, use standard input
        SetBinaryModeStdin(opt);
        ok = file.loadBinary(std::cin, opt, ts::CRC32::IGNORE);
    }
    else {
        ok = file.loadBinary(file_name, opt, ts::CRC32::IGNORE);
    }

    if (ok) {
        // Display all sections.
        ts::TablesDisplay display(opt.display, opt);
        for (ts::SectionPtrVector::const_iterator it = file.sections().begin(); it != file.sections().end(); ++it) {
            display.displaySection(**it) << std::endl;
        }
    }

    return ok;
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    TSDuckLibCheckVersion();
    Options opt(argc, argv);
    bool ok = true;

    std::cout << std::endl;

    if (opt.infiles.size() == 0) {
        ok = DumpFile(opt, u"");
    }
    else {
        for (ts::UStringVector::const_iterator it = opt.infiles.begin(); it != opt.infiles.end(); ++it) {
            ok = DumpFile(opt, *it) && ok;
        }
    }

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
