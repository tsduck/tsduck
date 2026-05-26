//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSValveArgs.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TSValveArgs::TSValveArgs(const UString& prefix) :
    _prefix(prefix.empty() || prefix.ends_with(u'-') ? prefix : prefix + u"-")
{
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TSValveArgs::defineArgs(Args& args)
{
    args.option((_prefix + u"preserve-units").c_str());
    args.help((_prefix + u"preserve-units").c_str(),
              u"Preserve payload unit boundaries (PUSI) when starting to pass or starting to drop packets. "
              u"When transitioning from passing to dropping packets, continue to pass each PID until the next payload unit boundary. "
              u"When transitioning from dropping to passing packets, start to pass each PID at the next payload unit boundary. "
              u"By default, the transport stream is abruptly cut at transition points.");

    args.option((_prefix + u"stuffing").c_str());
    args.help((_prefix + u"stuffing").c_str(),
              u"With --" + _prefix + u"preserve-unit, maintain the global bitrate of the transport stream during transition periods "
              u"by replacing dropped packets with null packets. "
              u"By default, during transition periods, the bitrate is gradually decreasing or increasing.");

    args.option((_prefix + u"silent-after").c_str(), 0, Args::POSITIVE);
    args.help((_prefix + u"silent-after").c_str(),
              u"With --" + _prefix + u"preserve-unit, specify the maximum number of packets after which a PID is considered as silent during a transition period. "
              u"The default is " + UString::Decimal(DEFAULT_SILENT_AFTER, 0, true, u"") + u" packets (" +
              UString::HumanSize(DEFAULT_SILENT_AFTER * PKT_SIZE * 8, u"b") + u").");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::TSValveArgs::loadArgs(Args& args)
{
    preserve_units = args.present((_prefix + u"preserve-units").c_str());
    stuffing = args.present((_prefix + u"stuffing").c_str());
    args.getIntValue(pid_max_silent, (_prefix + u"silent-after").c_str(), DEFAULT_SILENT_AFTER);
    return true;
}
