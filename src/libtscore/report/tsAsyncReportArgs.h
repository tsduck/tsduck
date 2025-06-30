//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Parameters and command line arguments for asynchronous log.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class Args;

    //!
    //! Parameters and command line arguments for asynchronous log.
    //! @ingroup libtscore cmd
    //!
    class TSCOREDLL AsyncReportArgs
    {
    public:
        // Public fields
        bool   sync_log = false;                  //!< Synchronous log.
        bool   timed_log = false;                 //!< Add time stamps in log messages.
        size_t log_msg_count = MAX_LOG_MESSAGES;  //!< Maximum buffered log messages.

        //!
        //! Default maximum number of messages in the queue.
        //! Must be limited since the logging thread has a low priority.
        //! If a high priority thread loops on report, it would exhaust the memory.
        //!
        static constexpr size_t MAX_LOG_MESSAGES = 512;

        //!
        //! Default constructor.
        //!
        AsyncReportArgs() = default;

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(Args& args);
    };
}
