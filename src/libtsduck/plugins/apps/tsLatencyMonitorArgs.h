//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Amos Cheung
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream latency monitor command-line options
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPluginOptions.h"

namespace ts {

    class Args;

    //!
    //! Transport stream latency monitor command-line options
    //! @ingroup plugin
    //!
    class TSDUCKDLL LatencyMonitorArgs
    {
    public:
        UString             appName {};          //!< Application name, for help messages.
        PluginOptionsVector inputs {};           //!< Input plugins descriptions.
        fs::path            outputName {};       //!< Output file name (empty means stderr).
        uint64_t            bufferTime = 0;      //!< Buffer time of timing data list
        uint64_t            outputInterval = 0;  //!< Waiting time between every output in seconds

        //!
        //! Constructor.
        //!
        LatencyMonitorArgs() = default;

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
