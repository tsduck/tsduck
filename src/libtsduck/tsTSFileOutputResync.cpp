//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//
//  A specialized form of transport stream output file with resynchronized
//  PID and continuity counters.
//
//----------------------------------------------------------------------------

#include "tsTSFileOutputResync.h"
#include "tsMemory.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::TSFileOutputResync::TSFileOutputResync() :
    TSFileOutput(),
    _ccFixer(AllPIDs)
{
    // Continuity counters are generated regardless of previous values.
    _ccFixer.setGenerator(true);
}

ts::TSFileOutputResync::~TSFileOutputResync()
{
}


//----------------------------------------------------------------------------
// Open method
//----------------------------------------------------------------------------

bool ts::TSFileOutputResync::open(const UString& filename, OpenFlags flags, Report& report)
{
    // Invoke superclass for actual file opening.
    const bool ok = TSFileOutput::open(filename, flags, report);

    // Reset continuity counters.
    if (ok) {
        _ccFixer.reset();
    }

    return ok;
}


//----------------------------------------------------------------------------
// Write packets, update their continuity counters (packets are modified)
//----------------------------------------------------------------------------

bool ts::TSFileOutputResync::write(TSPacket* buffer, size_t packet_count, Report& report)
{
    // Update continuity counters
    for (size_t n = 0; n < packet_count; ++n) {
        _ccFixer.feedPacket(buffer[n]);
    }

    // Invoke superclass
    return TSFileOutput::write(buffer, packet_count, report);
}


//----------------------------------------------------------------------------
// Write packets, force PID value
//----------------------------------------------------------------------------

bool ts::TSFileOutputResync::write(TSPacket* buffer, size_t packet_count, PID pid, Report& report)
{
    for (size_t n = 0; n < packet_count; ++n) {
        buffer[n].setPID(pid);
    }
    return write(buffer, packet_count, report);
}
