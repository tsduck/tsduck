//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  Declare the class ts::DektecVPD
//!
//-----------------------------------------------------------------------------

#pragma once
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
        static constexpr size_t VPD_SIZE = 64;

        // List of all VPD (always nul-terminated).
        char vpdid [VPD_SIZE];
        char cl    [VPD_SIZE];
        char ec    [VPD_SIZE];
        char mn    [VPD_SIZE];
        char pd    [VPD_SIZE];
        char pn    [VPD_SIZE];
        char sn    [VPD_SIZE];
        char xt    [VPD_SIZE];
        char bo    [VPD_SIZE];

        // Constructors
        DektecVPD() { clear(); }
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
