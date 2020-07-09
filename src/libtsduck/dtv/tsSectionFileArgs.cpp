//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsSectionFileArgs.h"
#include "tsArgs.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::SectionFileArgs::SectionFileArgs() :
    pack_and_flush(false),
    eit_normalize(false),
    eit_base_time()
{
}

ts::SectionFileArgs::~SectionFileArgs()
{
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::SectionFileArgs::defineArgs(Args& args) const
{
    args.option(u"eit-normalization");
    args.help(u"eit-normalization",
              u"Reorganize all EIT sections according to ETSI TS 101 211 rules. "
              u"One single EIT p/f subtable is built per service. It is split in two sections, "
              u"one for present and one for following events. "
              u"All EIT schedule are kept but they are completely reorganized. All events are "
              u"extracted and spread over new EIT sections according to ETSI TS 101 211 rules. "
              u"If several files are specified, the reorganization of EIT's is performed inside "
              u"each file independently. This is fine as long as all EIT's for a given service "
              u"are in the same input file. "
              u"See also option --eit-base-date.");

    args.option(u"eit-base-date", 0, Args::STRING);
    args.help(u"eit-base-date", u"date",
              u"With --eit-normalization, use the specified date as \"last midnight\" reference "
              u"for the allocation of the various EIT events in sections and segments. "
              u"The date must be in the format \"year/month/day\". "
              u"By default, use the oldest date in all EIT sections as base date.");

    args.option(u"pack-and-flush");
    args.help(u"pack-and-flush",
              u"When loading a binary section file, pack incomplete tables and flush them. "
              u"Sections are renumbered to remove any hole between sections. "
              u"Use with care because this may create inconsistent tables.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::SectionFileArgs::loadArgs(DuckContext& duck, Args& args)
{
    pack_and_flush = args.present(u"pack-and-flush");
    eit_normalize = args.present(u"eit-normalization");
    const UString date_str(args.value(u"eit-base-date"));

    if (!date_str.empty() && !eit_base_time.decode(date_str, Time::DATE)) {
        args.error(u"invalid date value \"%s\" (use \"year/month/day\")", {date_str});
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Process the content of a section file according to the selected options.
//----------------------------------------------------------------------------

bool ts::SectionFileArgs::processSectionFile(SectionFile& file, Report& report) const
{
    if (eit_normalize) {
        file.reorganizeEITs(eit_base_time);
    }

    if (pack_and_flush) {
        const size_t packed = file.packOrphanSections();
        if (packed > 0) {
            report.verbose(u"packed %d incomplete tables, may be invalid", {packed});
        }
    }

    return true;
}
