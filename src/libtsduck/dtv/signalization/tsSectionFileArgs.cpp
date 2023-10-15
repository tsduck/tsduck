//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSectionFileArgs.h"
#include "tsArgs.h"
#include "tsDuckContext.h"


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::SectionFileArgs::defineArgs(Args& args)
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
              u"With --eit-normalization, use the specified date as reference "
              u"for the allocation of the various EIT events in sections and segments. "
              u"The date must be in the format \"year/month/day [hh:mm:ss]\". "
              u"If only the date is present, it is used as base for the allocation of EIT schedule. "
              u"If the time is also specified, it is the current time for the snapshot of EIT present/following. "
              u"By default, use the oldest date in all EIT sections as base date.");

    args.option(u"eit-actual");
    args.help(u"eit-actual",
              u"With --eit-normalization, generate EIT actual. "
              u"Same as --eit-actual-pf --eit-actual-schedule.");

    args.option(u"eit-other");
    args.help(u"eit-other",
              u"With --eit-normalization, generate EIT other. "
              u"Same as --eit-other-pf --eit-other-schedule.");

    args.option(u"eit-pf");
    args.help(u"eit-pf",
              u"With --eit-normalization, generate EIT p/f. "
              u"Same as --eit-actual-pf --eit-other-pf.");

    args.option(u"eit-schedule");
    args.help(u"eit-schedule",
              u"With --eit-normalization, generate EIT schedule. "
              u"Same as --eit-actual-schedule --eit-other-schedule.");

    args.option(u"eit-actual-pf");
    args.help(u"eit-actual-pf",
              u"With --eit-normalization, generate EIT actual p/f. "
              u"If no option is specified, all EIT sections are generated.");

    args.option(u"eit-other-pf");
    args.help(u"eit-other-pf",
              u"With --eit-normalization, generate EIT other p/f. "
              u"If no option is specified, all EIT sections are generated.");

    args.option(u"eit-actual-schedule");
    args.help(u"eit-actual-schedule",
              u"With --eit-normalization, generate EIT actual schedule. "
              u"If no option is specified, all EIT sections are generated.");

    args.option(u"eit-other-schedule");
    args.help(u"eit-other-schedule",
              u"With --eit-normalization, generate EIT other schedule. "
              u"If no option is specified, all EIT sections are generated.");

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
    eit_base_time = Time::Epoch;

    // Try to decode --eit-base-date first as a date, then as date/time.
    const UString date_str(args.value(u"eit-base-date"));
    if (!date_str.empty() && !eit_base_time.decode(date_str, Time::DATE) && !eit_base_time.decode(date_str, Time::DATETIME)) {
        args.error(u"invalid date value \"%s\" (use \"year/month/day [hh:mm:ss]\")", {date_str});
        return false;
    }

    // Combination of EIT generation options.
    eit_options = EITOptions::GEN_NONE;
    if (args.present(u"eit-actual")) {
        eit_options |= EITOptions::GEN_ACTUAL;
    }
    if (args.present(u"eit-other")) {
        eit_options |= EITOptions::GEN_OTHER;
    }
    if (args.present(u"eit-pf")) {
        eit_options |= EITOptions::GEN_PF;
    }
    if (args.present(u"eit-schedule")) {
        eit_options |= EITOptions::GEN_SCHED;
    }
    if (args.present(u"eit-actual-pf")) {
        eit_options |= EITOptions::GEN_ACTUAL_PF;
    }
    if (args.present(u"eit-other-pf")) {
        eit_options |= EITOptions::GEN_OTHER_PF;
    }
    if (args.present(u"eit-actual-schedule")) {
        eit_options |= EITOptions::GEN_ACTUAL_SCHED;
    }
    if (args.present(u"eit-other-schedule")) {
        eit_options |= EITOptions::GEN_OTHER_SCHED;
    }
    if (!(eit_options & EITOptions::GEN_ALL)) {
        // Generate all sections by default.
        eit_options |= EITOptions::GEN_ALL;
    }

    return true;
}


//----------------------------------------------------------------------------
// Process the content of a section file according to the selected options.
//----------------------------------------------------------------------------

bool ts::SectionFileArgs::processSectionFile(SectionFile& file, Report& report) const
{
    if (eit_normalize) {
        file.reorganizeEITs(eit_base_time, eit_options);
    }

    if (pack_and_flush) {
        const size_t packed = file.packOrphanSections();
        if (packed > 0) {
            report.verbose(u"packed %d incomplete tables, may be invalid", {packed});
        }
    }

    return true;
}
