//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsAsyncReportArgs.h"
#include "tsArgs.h"

#if defined(TS_NEED_STATIC_CONST_DEFINITIONS)
const size_t ts::AsyncReportArgs::MAX_LOG_MESSAGES;
#endif


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::AsyncReportArgs::AsyncReportArgs() :
    sync_log(false),
    timed_log(false),
    log_msg_count(MAX_LOG_MESSAGES)
{
}


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

bool ts::AsyncReportArgs::loadArgs(DuckContext& duck, Args& args)
{
    log_msg_count = args.intValue<size_t>(u"log-message-count", MAX_LOG_MESSAGES);
    sync_log = args.present(u"synchronous-log");
    timed_log = args.present(u"timed-log");
    return true;
}
