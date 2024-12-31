//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Definition of TSP control commands syntax.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsCommandLine.h"

namespace ts {
    //!
    //! Definition of TSP control commands syntax.
    //! These commands are used with the @a tspcontrol utility to inspect or modify a running @a tsp command.
    //! @ingroup plugin
    //!
    class TSDUCKDLL TSPControlCommand : public CommandLine
    {
        TS_NOCOPY(TSPControlCommand);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Reference to a report where all messages are displayed.
        //! The reference must remain valid as long as this object exists.
        //!
        TSPControlCommand(Report& report = CERR);
    };
}
