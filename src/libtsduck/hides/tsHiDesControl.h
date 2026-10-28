//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A class implementing the @c tshides control utility.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsDuckContext.h"
#include "tsModulation.h"

namespace ts {
    //!
    //! A class implementing the tshides control utility.
    //! @ingroup libtsduck hardware
    //!
    class TSDUCKDLL HiDesControl: private Args
    {
        TS_NOBUILD_NOCOPY(HiDesControl);
    public:
        //!
        //! Constructor.
        //! @param [in] argc Command line argument count.
        //! @param [in] argv Command line arguments.
        //!
        HiDesControl(int argc, char *argv[]);

        //!
        //! Destructor.
        //!
        virtual ~HiDesControl() override;

        //!
        //! Execute the command.
        //! @return Either EXIT_SUCCESS or EXIT_FAILURE.
        //!
        int execute();

    private:
        DuckContext _duck {this};
        bool        _display_device_count = false;  // Only display device count.
        bool        _display_gain_range = false;    // Only display output gain range.
        int         _device_number = -1;            // Device adapter number.
        UString     _device_name {};                // Device name.
        uint64_t    _frequency = 0;                 // Carrier frequency, in Hz.
        BandWidth   _bandwidth = 0;                 // Bandwidth.
    };
}
