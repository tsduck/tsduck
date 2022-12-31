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
//!  Interface for classes listening for file modification.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPolledFile.h"

namespace ts {
    //!
    //! Interface for classes listening for file modification.
    //! @ingroup system
    //!
    class TSDUCKDLL PollFilesListener
    {
    public:
        //!
        //! Invoked when files have changed.
        //! @param [in] files List of changed files since last time.
        //! The entries in the list are sorted by file names.
        //! @return True to continue polling, false to exit polling.
        //!
        virtual bool handlePolledFiles(const PolledFileList& files) = 0;

        //!
        //! Invoked before each poll to give the opportunity to change where and how the files are polled.
        //! This is an optional feature, the default implementation does not change anything.
        //! @param [in,out] wildcard Wildcard specification of files to poll (eg "/path/to/*.dat").
        //! @param [in,out] poll_interval Interval in milliseconds between two poll operations.
        //! @param [in,out] min_stable_delay A file size needs to be stable during that duration
        //! for the file to be reported as added or modified. This prevent too frequent
        //! poll notifications when a file is being written and his size modified at each poll.
        //! @return True to continue polling, false to exit polling.
        //!
        virtual bool updatePollFiles(UString& wildcard, MilliSecond& poll_interval, MilliSecond& min_stable_delay);

        //!
        //! Virtual destructor.
        //!
        virtual ~PollFilesListener();
    };
}
