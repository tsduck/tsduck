//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAsyncReportArgs.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::AsyncReportArgs::defineArgs(Args& args)
{
    args.option(u"log-message-count", 0, Args::POSITIVE);
    args.help(u"log-message-count",
              u"Specify the maximum number of buffered log messages. Log messages are "
              u"displayed asynchronously in a low priority thread. This value specifies "
              u"the maximum number of buffered log messages in memory, before being "
              u"displayed. When too many messages are logged in a short period of time, "
              u"while plugins use all CPU power, extra messages are dropped. Increase "
              u"this value if you think that too many messages are dropped. The default "
              u"is " + UString::Decimal(MAX_LOG_MESSAGES) + u" messages.");

    args.option(u"synchronous-log", 's');
    args.help(u"synchronous-log",
              u"Each logged message is guaranteed to be displayed, synchronously, without "
              u"any loss of message. The downside is that a plugin thread may be blocked "
              u"for a short while when too many messages are logged. This option shall be "
              u"used when all log messages are needed and the source and destination are "
              u"not live streams (files for instance). This option is not recommended for "
              u"live streams, when the responsiveness of the application is more important "
              u"than the logged messages.");

    args.option(u"timed-log", 't');
    args.help(u"timed-log", u"Each logged message contains a time stamp.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::AsyncReportArgs::loadArgs(Args& args)
{
    args.getIntValue(log_msg_count, u"log-message-count", MAX_LOG_MESSAGES);
    sync_log = args.present(u"synchronous-log");
    timed_log = args.present(u"timed-log");
    return true;
}
