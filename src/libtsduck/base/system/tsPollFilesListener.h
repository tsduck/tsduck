//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
