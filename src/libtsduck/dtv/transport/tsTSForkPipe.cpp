//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSForkPipe.h"


//----------------------------------------------------------------------------
// Constructor / destructor
//----------------------------------------------------------------------------

ts::TSForkPipe::TSForkPipe(Report* report, Object* owner) :
    ForkPipe(report, owner),
    TSPacketStream(*static_cast<ReporterBase*>(this))
{
}

ts::TSForkPipe::TSForkPipe(ReporterBase* delegate, Object* owner) :
    ForkPipe(delegate, owner),
    TSPacketStream(*static_cast<ReporterBase*>(this))
{
}

ts::TSForkPipe::~TSForkPipe()
{
}


//----------------------------------------------------------------------------
// Create the process, open the optional pipe.
//----------------------------------------------------------------------------

bool ts::TSForkPipe::open(const UString& command, WaitMode wait_mode, size_t buffer_size, OutputMode out_mode, InputMode in_mode, TSPacketFormat format)
{
    resetPacketStream(format, this);
    return ForkPipe::open(command, wait_mode, buffer_size, out_mode, in_mode);
}
