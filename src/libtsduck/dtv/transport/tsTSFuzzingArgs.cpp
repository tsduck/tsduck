//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard, Sergey Lobanov
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------

#include "tsTSFuzzingArgs.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::TSFuzzingArgs::defineArgs(Args& args)
{
    args.option(u"pid", 'p', Args::PIDVAL, 0, Args::UNLIMITED_COUNT);
    args.help(u"pid", u"pid1[-pid2]",
              u"Corrupt only packets with these PID values. "
              u"Several --pid options may be specified. "
              u"By default, all packets can be corrupted.");

    args.option<decltype(probability)>(u"corrupt-probability", 'c');
    args.help(u"corrupt-probability",
              u"Probability to corrupt a byte in the transport stream. "
              u"The default is zero, meaning no corruption.");

    args.option(u"seed", 's', Args::HEXADATA);
    args.help(u"seed",
              u"Initial seed for the pseudo-random number generator. "
              u"Specify hexadecimal data. The size is not limited but at least 32 bytes are recommended. "
              u"Using the same seed on the same TS file will result in exactly the same corruptions. "
              u"Without this parameter, a random seed is used and the corruptions cannot be identically reproduced.");

    args.option(u"sync-byte");
    args.help(u"sync-byte",
              u"May corrupt the 0x47 sync byte in TS packets. "
              u"This may invalidate the synchronization of the transport stream. "
              u"By default, sync bytes are preserved.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::TSFuzzingArgs::loadArgs(DuckContext& duck, Args& args)
{
    sync_byte = args.present(u"sync-byte");
    args.getValue(probability, u"corrupt-probability");
    args.getIntValues(pids, u"pid", true);
    args.getHexaValue(seed, u"seed");
    return true;
}
