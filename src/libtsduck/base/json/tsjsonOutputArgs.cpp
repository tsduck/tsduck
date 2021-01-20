//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2021, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsjsonOutputArgs.h"
#include "tsArgs.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::json::OutputArgs::OutputArgs(bool use_short_opt, const UString& help) :
    json(false),
    json_line(false),
    json_prefix(),
    _use_short_opt(use_short_opt),
    _json_help(help.empty() ? u"Report in JSON output format (useful for automatic analysis)." : help)
{
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::json::OutputArgs::defineArgs(Args& args) const
{
    args.option(u"json", _use_short_opt ? 'j' : 0);
    args.help(u"json", _json_help);

    args.option(u"json-line", 0, Args::STRING, 0, 1, 0, Args::UNLIMITED_VALUE, true);
    args.help(u"json-line", u"'prefix'",
              u"Same as --json but report the JSON text as one single line "
              u"in the message logger instead of the output file. "
              u"The optional string parameter specifies a prefix to prepend on the log "
              u"line before the JSON text to locate the appropriate line in the logs.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

bool ts::json::OutputArgs::loadArgs(DuckContext& duck, Args& args)
{
    json_line = args.present(u"json-line");
    json = json_line || args.present(u"json");
    args.getValue(json_prefix, u"json-line");
    return true;
}


//----------------------------------------------------------------------------
// Issue a JSON report according to options.
//----------------------------------------------------------------------------

void ts::json::OutputArgs::report(const json::Value& root, std::ostream& stm, Report& rep) const
{
    // An output text formatter for JSON output.
    TextFormatter text(rep);

    if (json_line) {
        // Generate one line.
        text.setString();
        text.setEndOfLineMode(TextFormatter::EndOfLineMode::SPACING);
        root.print(text);
        rep.info(json_prefix + text.toString());
    }
    else if (json) {
        // Output to stream.
        text.setStream(stm);
        root.print(text);
        text << ts::endl;
        text.close();
    }
}
