//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command line arguments for transport stream packets dump.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTS.h"

namespace ts {

    class Args;
    class DuckContext;

    //!
    //! Command line arguments for transport stream packets dump.
    //! @ingroup cmd
    //!
    class TSDUCKDLL TSDumpArgs
    {
        TS_NOCOPY(TSDumpArgs);
    public:
        // Public fields
        uint32_t dump_flags = 0;  //!< Dump options for Hexa and Packet::dump
        bool     log = false;     //!< Option -\-log
        size_t   log_size = 0;    //!< Size to display with -\-log
        PIDSet   pids {};         //!< PID values to dump

        //!
        //! Default constructor.
        //!
        TSDumpArgs() = default;

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
