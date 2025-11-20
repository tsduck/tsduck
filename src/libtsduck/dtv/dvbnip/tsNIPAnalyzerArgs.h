//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command line arguments for the class NIPAnalyzer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class Args;
    class DuckContext;

    //!
    //! Command line arguments for the class NIPAnalyzer.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL NIPAnalyzerArgs
    {
    public:
        //!
        //! Constructor.
        //!
        NIPAnalyzerArgs() = default;

        // Full analysis options:
        bool log_flute_packets = false;   //!< Option -\-log-flute-packets
        bool dump_flute_payload = false;  //!< Option -\-dump-flute-payload
        bool log_fdt = false;             //!< Option -\-log-fdt

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(DuckContext& duck, Args& args);
    };
}
