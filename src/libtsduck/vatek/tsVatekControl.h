//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2024, Vision Advance Technology Inc. (VATek)
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A class implementing the @c tsvatek control utility.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsDuckContext.h"

namespace ts {
    //!
    //! A class implementing the tsvatek control utility.
    //! This is defined as a separate class the interface of which does not depend on the Vatek core library.
    //! @ingroup hardware
    //!
    class TSDUCKDLL VatekControl: private Args
    {
        TS_NOBUILD_NOCOPY(VatekControl);
    public:
        //!
        //! Constructor.
        //! @param [in] argc Command line argument count.
        //! @param [in] argv Command line arguments.
        //!
        VatekControl(int argc, char *argv[]);

        //!
        //! Destructor.
        //!
        virtual ~VatekControl() override;

        //!
        //! Execute the command.
        //! @return Either EXIT_SUCCESS or EXIT_FAILURE.
        //!
        int execute();

    private:
        DuckContext _duck {this};
        int32_t     _dev_index = -100;  // Vatek device index, -100 for all devices
    };
}
