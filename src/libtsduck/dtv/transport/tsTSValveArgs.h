//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command line arguments for TSValve.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {

    class Args;

    //!
    //! Command line arguments for TSValve.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL TSValveArgs
    {
    public:
        //!
        //! Constructor.
        //! @param [in] prefix Use that prefix for all long option (e.g. '--foo-stuffing' for '--stuffing').
        //!
        TSValveArgs(const UString& prefix = UString());

        // Public fields, by options.
        bool preserve_units = false;  //!< -\-preserve-units
        bool stuffing = false;        //!< -\-stuffing

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
