//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A class implementing the @c tsdektec control utility.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsDuckContext.h"

namespace ts {
    //!
    //! A class implementing the tsdektec control utility.
    //! This is defined as a separate class the interface of which does not depend on DTAPI.
    //! The binary DTAPI is privately isolated inside the TSDuck library.
    //! @ingroup libtsduck hardware
    //!
    class TSDEKTECDLL DektecControl: private Args
    {
        TS_NOBUILD_NOCOPY(DektecControl);
    public:
        //!
        //! Constructor.
        //! @param [in] argc Command line argument count.
        //! @param [in] argv Command line arguments.
        //!
        DektecControl(int argc, char *argv[]);

        //!
        //! Destructor.
        //!
        virtual ~DektecControl() override;

        //!
        //! Execute the command.
        //! @return Either EXIT_SUCCESS or EXIT_FAILURE.
        //!
        int execute();

    private:
        // Redirect it to an internal DTAPI-dependent "guts" class.
        class Guts;
        DuckContext _duck {this};
        Guts* _guts = nullptr;
    };
}
