//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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

#pragma once
#include "tsTSFileOutput.h"

namespace ts {

    class TSDUCKDLL TSFileOutputResync: public TSFileOutput
    {
    public:
        // Constructor / destructor
        TSFileOutputResync() : TSFileOutput() {}
        virtual ~TSFileOutputResync() {}

        // Overrides TSFileOutput methods
        bool open (const std::string& filename, bool append, bool keep, ReportInterface&);

        // Write packets, update their continuity counters (packets are modified)
        bool write (TSPacket*, size_t packet_count, ReportInterface&);

        // Write packets, force PID value, update their continuity counters (packets are modified)
        bool write (TSPacket*, size_t packet_count, PID, ReportInterface&);

    private:
        // Private members
        uint8_t _cc[PID_MAX];  // Last continuity counter per PID

        // Inaccessible operations
        TSFileOutputResync (const TSFileOutputResync&);
        TSFileOutputResync& operator= (const TSFileOutputResync&);
    };
}
