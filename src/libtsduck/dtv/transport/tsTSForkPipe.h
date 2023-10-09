//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
