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
//  Options for the class PSILogger.
//
//----------------------------------------------------------------------------

#include "tsPSILoggerArgs.h"
#include "tsException.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::PSILoggerArgs::PSILoggerArgs() :
    SuperClass(),
    all_versions(false),
    clear(false),
    cat_only(false),
    dump(false),
    output()
{
}


//----------------------------------------------------------------------------
// Add help about command line options in an Args
//----------------------------------------------------------------------------

void ts::PSILoggerArgs::addHelp(Args& args) const
{
    std::string help =
        "\n"
        "PSI logging options:\n"
        "\n"
        "  -a\n"
        "  --all-versions\n"
        "      Display all versions of PSI tables (need to read the complete\n"
        "      transport stream). By default, display only the first version\n"
        "      of each PSI table and stop when all expected PSI are extracted.\n"
        "\n"
        "  --cat-only\n"
        "      Display only the CAT, ignore other PSI tables.\n"
        "\n"
        "  -c\n"
        "  --clear\n"
        "      Indicate that this is a clear transport stream, without\n"
        "      conditional access information. Useful to avoid reading the\n"
        "      complete transport stream, waiting for a non-existent CAT.\n"
        "\n"
        "  -d\n"
        "  --dump\n"
        "      Dump all PSI sections.\n"
        "\n"
        "  --help\n"
        "      Display this help text.\n"
        "\n"
        "  -o filename\n"
        "  --output-file filename\n"
        "      File name for text output.\n"
        "\n"
        "  -v\n"
        "  --verbose\n"
        "      Produce verbose output.\n"
        "\n"
        "  --version\n"
        "      Display the version number.\n";

    args.setHelp(args.getHelp() + help);

    // Let superclass add its own help.
    SuperClass::addHelp(args);
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::PSILoggerArgs::defineOptions(Args& args) const
{
    args.option("all-versions", 'a');
    args.option("cat-only",      0);
    args.option("clear",        'c');
    args.option("debug",         0, Args::POSITIVE, 0, 1, 0, 0, true);
    args.option("dump",         'd');
    args.option("verbose",      'v');
    args.option("output-file",  'o', Args::STRING);

    // Let superclass defines its own options.
    SuperClass::defineOptions(args);
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

void ts::PSILoggerArgs::load(Args& args)
{
    args.setDebugLevel(args.present("debug") ? args.intValue("debug", Severity::Debug) : args.present("verbose") ? Severity::Verbose : Severity::Info);
    all_versions = args.present("all-versions");
    clear = args.present("clear");
    cat_only = args.present("cat-only");
    dump = args.present("dump");
    output = args.value("output-file");
}
