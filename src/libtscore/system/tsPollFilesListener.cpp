//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPollFilesListener.h"

bool ts::PollFilesListener::updatePollFiles(UString& wildcard, cn::milliseconds& poll_interval, cn::milliseconds& min_stable_delay)
{
    return true;
}

ts::PollFilesListener::~PollFilesListener()
{
}
