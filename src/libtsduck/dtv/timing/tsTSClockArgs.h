//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command line arguments for TSClock.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsTime.h"

namespace ts {

    class Args;

    //!
    //! Command line arguments for TSClock.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL TSClockArgs
    {
    public:
        //!
        //! Constructor.
        //! @param [in] prefix Use that prefix for all long option (e.g. '--foo-local-time' for '--local-time').
        //!
        TSClockArgs(const UString& prefix = UString());

        // Public fields, by options.
        bool pcr_based = false;        //!< -\-pcr-based
        bool timestamp_based = false;  //!< -\-timestamp-based
        bool use_local_time = false;   //!< -\-local-time
        Time start_time {};            //!< -\-start-time (always UTC)

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(Args& args);

    private:
        UString _prefix {};
    };
}
