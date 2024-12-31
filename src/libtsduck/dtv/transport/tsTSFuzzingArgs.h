//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard, Sergey Lobanov
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command line arguments for transport stream packets fuzzing.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsByteBlock.h"
#include "tsFraction.h"
#include "tsTS.h"

namespace ts {

    class Args;
    class DuckContext;

    //!
    //! Command line arguments for transport stream packets fuzzing.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TSFuzzingArgs
    {
    public:
        // Public fields
        bool               sync_byte = false;  //!< May corrupt the 0x47 sync byte in TS packets.
        PIDSet             pids {};            //!< PID values which can be corrupted.
        Fraction<uint32_t> probability = 0;    //!< Probability of corrupting a byte in the stream.
        ByteBlock          seed {};            //!< Seed for the PRNG, required for reproducibility.

        //!
        //! Constructor.
        //!
        TSFuzzingArgs() = default;

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
