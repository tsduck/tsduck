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
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::PSILoggerArgs::PSILoggerArgs() :
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
    const UString help =
        u"\n"
        u"PSI logging options:\n"
        u"\n"
        u"  -a\n"
        u"  --all-versions\n"
        u"      Display all versions of PSI tables (need to read the complete\n"
        u"      transport stream). By default, display only the first version\n"
        u"      of each PSI table and stop when all expected PSI are extracted.\n"
        u"\n"
        u"  --cat-only\n"
        u"      Display only the CAT, ignore other PSI tables.\n"
        u"\n"
        u"  -c\n"
        u"  --clear\n"
        u"      Indicate that this is a clear transport stream, without\n"
        u"      conditional access information. Useful to avoid reading the\n"
        u"      complete transport stream, waiting for a non-existent CAT.\n"
        u"\n"
        u"  -d\n"
        u"  --dump\n"
        u"      Dump all PSI sections.\n"
        u"\n"
        u"  --help\n"
        u"      Display this help text.\n"
        u"\n"
        u"  -o filename\n"
        u"  --output-file filename\n"
        u"      File name for text output.\n"
        u"\n"
        u"  -v\n"
        u"  --verbose\n"
        u"      Produce verbose output.\n"
        u"\n"
        u"  --version\n"
        u"      Display the version number.\n";

    args.setHelp(args.getHelp() + help);
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::PSILoggerArgs::defineOptions(Args& args) const
{
    args.option(u"all-versions", 'a');
    args.option(u"cat-only",      0);
    args.option(u"clear",        'c');
    args.option(u"debug",         0, Args::POSITIVE, 0, 1, 0, 0, true);
    args.option(u"dump",         'd');
    args.option(u"verbose",      'v');
    args.option(u"output-file",  'o', Args::STRING);
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

void ts::PSILoggerArgs::load(Args& args)
{
    args.setDebugLevel(args.present(u"debug") ? args.intValue(u"debug", Severity::Debug) : args.present(u"verbose") ? Severity::Verbose : Severity::Info);
    all_versions = args.present(u"all-versions");
    clear = args.present(u"clear");
    cat_only = args.present(u"cat-only");
    dump = args.present(u"dump");
    output = args.value(u"output-file");
}
