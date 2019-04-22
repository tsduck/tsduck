//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::PSILoggerArgs::PSILoggerArgs() :
    all_versions(false),
    clear(false),
    cat_only(false),
    dump(false),
    output(),
    use_current(true),
    use_next(false)
{
}

ts::PSILoggerArgs::~PSILoggerArgs()
{
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::PSILoggerArgs::defineOptions(Args& args) const
{
    args.option(u"all-versions", 'a');
    args.help(u"all-versions",
              u"Display all versions of PSI tables (need to read the complete "
              u"transport stream). By default, display only the first version "
              u"of each PSI table and stop when all expected PSI are extracted.");

    args.option(u"cat-only");
    args.help(u"cat-only", u"Display only the CAT, ignore other PSI tables.");

    args.option(u"clear", 'c');
    args.help(u"clear",
              u"Indicate that this is a clear transport stream, without "
              u"conditional access information. Useful to avoid reading the "
              u"complete transport stream, waiting for a non-existent CAT.");

    args.option(u"dump", 'd');
    args.help(u"dump", u"Dump all PSI sections.");

    args.option(u"exclude-current");
    args.help(u"exclude-current",
              u"Exclude PSI tables with \"current\" indicator. "
              u"This is rarely necessary. See also --include-next.");

    args.option(u"include-next");
    args.help(u"include-next",
              u"Include PSI tables with \"next\" indicator. By default, they are excluded.");

    args.option(u"output-file", 'o', Args::STRING);
    args.help(u"output-file", u"File name for text output.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

bool ts::PSILoggerArgs::load(Args& args)
{
    all_versions = args.present(u"all-versions");
    cat_only = args.present(u"cat-only");
    clear = args.present(u"clear");
    dump = args.present(u"dump");
    output = args.value(u"output-file");
    use_current = !args.present(u"exclude-current");
    use_next = args.present(u"include-next");
    return true;
}
