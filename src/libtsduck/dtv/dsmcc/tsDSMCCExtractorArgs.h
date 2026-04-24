//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Piotr Serafin
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command-line options for DSMCCExtractor (shared by plugin and tool).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsDSMCCExtractor.h"

namespace ts {
    //!
    //! Command-line options for @ref DSMCCExtractor.
    //!
    //! Consolidates the argument definitions, parsing and validation shared by
    //! the `dsmcc` plugin and the `tsdsmcc` standalone command. Adding a new
    //! option happens in one place.
    //!
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL DSMCCExtractorArgs
    {
    public:
        PID                      pid = PID_NULL;  //!< PID carrying the carousel.
        DSMCCExtractor::Options  options {};      //!< Extraction policy.

        //!
        //! Default constructor.
        //!
        DSMCCExtractorArgs() = default;

        //!
        //! Define the command-line options on @a args.
        //! @param [in,out] args The Args subclass (plugin or command).
        //!
        void defineArgs(Args& args);

        //!
        //! Load parsed values from @a args into this structure and validate
        //! option combinations.
        //! @param [in,out] args The Args subclass (plugin or command).
        //! @return True on success, false if validation failed.
        //!
        bool loadArgs(Args& args);
    };
}  // namespace ts
