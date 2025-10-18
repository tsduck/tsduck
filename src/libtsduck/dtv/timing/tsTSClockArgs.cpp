//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSClockArgs.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TSClockArgs::TSClockArgs(const UString& prefix) :
    _prefix(prefix.empty() || prefix.ends_with(u'-') ? prefix : prefix + u"-")
{
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TSClockArgs::defineArgs(Args& args)
{
    args.option((_prefix + u"local-time").c_str());
    args.help((_prefix + u"local-time").c_str(),
              u"Interpret timestamps as local time, based on the current system configuration. "
              u"By default, timestamps are interpreted as UTC time.");

    args.option((_prefix + u"pcr-based").c_str());
    args.help((_prefix + u"pcr-based").c_str(),
         u"Use playout time based on PCR values. "
         u"By default, the time is based on the wall-clock time (real time).");

    args.option((_prefix + u"timestamp-based").c_str());
    args.help((_prefix + u"timestamp-based").c_str(),
         u"Use playout time based on timestamp values from the input plugin. "
         u"When input timestamps are not available or not monotonic, fallback to --" + _prefix + u"pcr-based. "
         u"By default, the time is based on the wall-clock time (real time).");

    args.option((_prefix + u"start-time").c_str(), 0, Args::STRING);
    args.help((_prefix + u"start-time").c_str(), u"year/month/day:hour:minute:second",
         u"With --" + _prefix + u"pcr-based or --" + _prefix + u"timestamp-based, specify the initial date & time reference. "
         u"By default, with --" + _prefix + u"pcr-based or --" + _prefix + u"timestamp-based, "
         u"the activity starts at the first UTC time which is found in a DVB TDT or ATSC STT.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::TSClockArgs::loadArgs(Args& args)
{
    pcr_based = args.present((_prefix + u"pcr-based").c_str());
    timestamp_based = args.present((_prefix + u"timestamp-based").c_str());
    use_local_time = args.present((_prefix + u"local-time").c_str());
    start_time = Time::Epoch;

    const UString opt_start_time(_prefix + u"start-time");
    const UString val_start_time(args.value(opt_start_time.c_str()));
    if (!val_start_time.empty()) {
        if (!start_time.decode(val_start_time)) {
            args.error(u"invalid --%s value \"%s\" (use \"year/month/day:hour:minute:second\")", opt_start_time, val_start_time);
            return false;
        }
        else if (use_local_time) {
            // The specified time is local but we use UTC internally.
            start_time = start_time.localToUTC();
        }
    }

    return true;
}
