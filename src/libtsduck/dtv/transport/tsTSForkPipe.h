//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  A subclass of ts::ForkPipe which exchanges TS packets on the pipe.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsForkPipe.h"
#include "tsTSPacketStream.h"

namespace ts {
    //!
    //! A subclass of ts::ForkPipe which exchanges TS packets on the pipe.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TSForkPipe: public ForkPipe, public TSPacketStream
    {
        TS_NOCOPY(TSForkPipe);
    public:
        //!
        //! Default constructor.
        //!
        TSForkPipe();

        //!
        //! Destructor.
        //!
        virtual ~TSForkPipe() override;

        //!
        //! Create the process, open the optional pipe.
        //! @param [in] command The command to execute.
        //! @param [in] wait_mode How to wait for process termination in close().
        //! @param [in] buffer_size The pipe buffer size in bytes. Used on Windows only. Zero means default.
        //! @param [in,out] report Where to report errors.
        //! @param [in] out_mode How to handle stdout and stderr.
        //! @param [in] in_mode How to handle stdin. Use the pipe by default.
        //! When set to KEEP_STDIN, no pipe is created.
        //! @param [in] format Format of the TS streams over the pipe.
        //! @return True on success, false on error.
        //! Do not return on success when @a wait_mode is EXIT_PROCESS.
        //!
        bool open(const UString& command,
                  WaitMode wait_mode,
                  size_t buffer_size,
                  Report& report,
                  OutputMode out_mode,
                  InputMode in_mode,
                  TSPacketFormat format = TSPacketFormat::AUTODETECT);
    };
}
