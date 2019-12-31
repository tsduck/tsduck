//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A class implementing the @c tsdektec control utility.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"

#if !defined(DOXYGEN)
// Forward definitions for private part only.
namespace Dtapi {
    struct DtHwFuncDesc;
}
namespace ts {
    class DektecDevice;
    typedef std::vector<DektecDevice> DektecDeviceVector;
}
#endif

namespace ts {

    //!
    //! A class implementing the tsdektec control utility.
    //! This is defined as a separate class the interface of which does not
    //! depend on DTAPI. The binary DTAPI is privately isolated inside tsduck.dll/.so.
    //! @ingroup hardware
    //!
    class TSDUCKDLL DektecControl: public Args
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
        //! Virtual destructor.
        //!
        virtual ~DektecControl();

        //!
        //! Execute the command.
        //! @return Either EXIT_SUCCESS or EXIT_FAILURE.
        //!
        int execute();

    private:
        // Command line parameters.
        bool   _list_all;      //!< List all Dektec devices
        bool   _normalized;    //!< List in "normalized" format
        int    _wait_sec;      //!< Wait time before exit
        size_t _devindex;      //!< Dektec device
        bool   _reset;         //!< Reset the device
        bool   _set_led;       //!< Change LED state
        int    _led_state;     //!< State of the LED (one of DTAPI_LED_*)
        int    _set_input;     //!< Port number to set as input, for directional ports
        int    _set_output;    //!< Port number to set as output, for directional ports
        int    _power_mode;    //!< Power mode to set on DTU-315

        // Apply commands to one device. Return command status.
        int oneDevice(const DektecDevice& device);

        // Displays a list of all Dektec devices. Return command status.
        int listDevices(const DektecDeviceVector& devices);

        // Displays a list of all Dektec devices in normalized format. Return command status.
        int listNormalizedDevices(const DektecDeviceVector& devices);

        // Displays the capability of a hardware function in normalized format.
        void listNormalizedCapabilities(size_t device_index, size_t channel_index, const char* type, const Dtapi::DtHwFuncDesc& hw);

        // Display a long line on multiple lines
        void wideDisplay(const UString& line);
    };
}
