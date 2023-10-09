//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  Declare the class ts::DektecVPD
//!
//-----------------------------------------------------------------------------

#pragma once
#if !defined(TS_NO_DTAPI) || defined(DOXYGEN)

#include "tsPlatform.h"
#include "tsDektec.h"

namespace ts {
    //!
    //! Description of a Dektec device's Vital Product Data (VPD).
    //!
    class DektecVPD
    {
    public:
        // Size of one VPD entry
        static const size_t VPD_SIZE = 64;

        // List of all VPD (always nul-terminated).
        char vpdid [VPD_SIZE];  // Flawfinder: ignore
        char cl    [VPD_SIZE];  // Flawfinder: ignore
        char ec    [VPD_SIZE];  // Flawfinder: ignore
        char mn    [VPD_SIZE];  // Flawfinder: ignore
        char pd    [VPD_SIZE];  // Flawfinder: ignore
        char pn    [VPD_SIZE];  // Flawfinder: ignore
        char sn    [VPD_SIZE];  // Flawfinder: ignore
        char xt    [VPD_SIZE];  // Flawfinder: ignore
        char bo    [VPD_SIZE];  // Flawfinder: ignore

        // Constructors
        DektecVPD() {clear();}
        DektecVPD(const Dtapi::DtDeviceDesc& dev) {get(dev);}
        DektecVPD(Dtapi::DtDevice& dev) {get(dev);}

        // Get VPD from a device
        void get(const Dtapi::DtDeviceDesc&);
        void get(Dtapi::DtDevice&);

        // Clear content
        void clear();

    private:
        // Clear one entry from Vital Product Data.
        void clearOne(char* data);

        // Read one entry from Vital Product Data. Make sure it is nul-terminated
        void getOneVPD(Dtapi::DtDevice& dev, const char* keyword, char* data);
    };
}

#endif // TS_NO_DTAPI
