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
//!
//!  @file
//!  A specialized form of transport stream output file with resynchronized
//!  PID and continuity counters.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSFileOutput.h"

namespace ts {
    //!
    //! A specialized form of transport stream output file with resynchronized PID and continuity counters.
    //!
    //! On each PID, the continuity counters are automatically updated and synchronized.
    //! It is also possible to force the PID of packets.
    //!
    class TSDUCKDLL TSFileOutputResync: public TSFileOutput
    {
    public:
        //!
        //! Default constructor.
        //!
        TSFileOutputResync() : TSFileOutput() {}

        //!
        //! Destructor.
        //!
        virtual ~TSFileOutputResync() {}

        // Overrides TSFileOutput methods
        virtual bool open(const std::string& filename, bool append, bool keep, ReportInterface& report);

        //!
        //! Write TS packets to the file.
        //! @param [in,out] buffer Address of first packet to write.
        //! The continuity counters of all packets are modified.
        //! @param [in] packet_count Number of packets to write.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool write(TSPacket* buffer, size_t packet_count, ReportInterface& report);

        //!
        //! Write TS packets to the file.
        //! @param [in,out] buffer Address of first packet to write.
        //! The continuity counters of all packets are modified.
        //! @param [in] packet_count Number of packets to write.
        //! @param [in] pid The PID of all packets is forced to this value.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool write(TSPacket* buffer, size_t packet_count, PID pid, ReportInterface& report);

    private:
        // Private members
        uint8_t _cc[PID_MAX];  // Last continuity counter per PID

        // Inaccessible operations
        TSFileOutputResync(const TSFileOutputResync&) = delete;
        TSFileOutputResync& operator=(const TSFileOutputResync&) = delete;
    };
}
