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
//!  Poll for files.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCerrReport.h"
#include "tsPollFilesListener.h"

namespace ts {
    //!
    //! A class to poll files for modifications.
    //! @ingroup system
    //!
    class TSDUCKDLL PollFiles
    {
        TS_NOCOPY(PollFiles);
    public:
        //!
        //! Default interval in milliseconds between two poll operations.
        //!
        static const MilliSecond DEFAULT_POLL_INTERVAL = 1000;

        //!
        //! Default minimum file stability delay.
        //! A file size needs to be stable during that duration for the file to be reported as
        //! added or modified. This prevent too frequent poll notifications when a file is
        //! being written and his size modified at each poll.
        //!
        static const MilliSecond DEFAULT_MIN_STABLE_DELAY = 500;

        //!
        //! Constructor.
        //! @param [in] wildcard Wildcard specification of files to poll (eg "/path/to/*.dat").
        //! @param [in] listener Invoked on file modification. Can be null.
        //! @param [in] poll_interval Interval in milliseconds between two poll operations.
        //! @param [in] min_stable_delay A file size needs to be stable during that duration
        //! for the file to be reported as added or modified. This prevent too frequent
        //! poll notifications when a file is being written and his size modified at each poll.
        //! @param [in,out] report For debug messages only.
        //!
        PollFiles(const UString& wildcard = UString(),
                  PollFilesListener* listener = nullptr,
                  MilliSecond poll_interval = DEFAULT_POLL_INTERVAL,
                  MilliSecond min_stable_delay = DEFAULT_MIN_STABLE_DELAY,
                  Report& report = CERR);

        //!
        //! Set a new file wildcard specification to poll.
        //! @param [in] wildcard Wildcard specification of files to poll (eg "/path/to/*.dat").
        //!
        void setFileWildcard(const UString& wildcard) { _files_wildcard = wildcard; }

        //!
        //! Set a new file listener.
        //! @param [in] listener Invoked on file modification. Can be null.
        //!
        void setListener(PollFilesListener* listener) { _listener = listener; }

        //!
        //! Set a new polling interval.
        //! @param [in] poll_interval Interval in milliseconds between two poll operations.
        //!
        void setPollInterval(MilliSecond poll_interval) { _poll_interval = poll_interval; }

        //!
        //! Set a new minimum file stability delay.
        //! @param [in] min_stable_delay A file size needs to be stable during that duration
        //! for the file to be reported as added or modified. This prevent too frequent
        //! poll notifications when a file is being written and his size modified at each poll.
        //!
        void setMinStableDelay(MilliSecond min_stable_delay) { _min_stable_delay = min_stable_delay; }

        //!
        //! Poll files continuously until the listener asks to terminate.
        //! Invoke the listener each time something has changed.
        //! The first time, all files are reported as "added".
        //!
        void pollRepeatedly();

        //!
        //! Perform one poll operation, notify listener if necessary, and return immediately.
        //! @return True to continue polling, false to exit polling.
        //!
        bool pollOnce();

    private:
        UString            _files_wildcard;
        Report&            _report;
        MilliSecond        _poll_interval;
        MilliSecond        _min_stable_delay;
        PollFilesListener* _listener;
        PolledFileList     _polled_files;   // Updated at each poll, sorted by file name
        PolledFileList     _notified_files; // Modifications to notify

        // Mark a file as deleted, move from polled to notified files.
        void deleteFile(PolledFileList::iterator&);
    };
}
