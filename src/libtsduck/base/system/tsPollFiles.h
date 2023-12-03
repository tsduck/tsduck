//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        static constexpr MilliSecond DEFAULT_POLL_INTERVAL = 1000;

        //!
        //! Default minimum file stability delay.
        //! A file size needs to be stable during that duration for the file to be reported as
        //! added or modified. This prevent too frequent poll notifications when a file is
        //! being written and his size modified at each poll.
        //!
        static constexpr MilliSecond DEFAULT_MIN_STABLE_DELAY = 500;

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
        Report&            _report;
        UString            _files_wildcard {};
        MilliSecond        _poll_interval {DEFAULT_POLL_INTERVAL};
        MilliSecond        _min_stable_delay {DEFAULT_MIN_STABLE_DELAY};
        PollFilesListener* _listener = nullptr;
        PolledFileList     _polled_files {};   // Updated at each poll, sorted by file name
        PolledFileList     _notified_files {}; // Modifications to notify

        // Mark a file as deleted, move from polled to notified files.
        void deleteFile(PolledFileList::iterator&);
    };
}
