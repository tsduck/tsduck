//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//-----------------------------------------------------------------------------
//!
//!  @file
//!  Declare the class ts::DektecVPD
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsDektec.h"

#if !defined(TS_NO_DTAPI)

namespace ts {

    //!
    //! Description of a Dektec device's Vital Product Data (VPD).
    //!
    class DektecVPD
    {
    public:
        // Size of one VPD entry
        static const size_t VPD_SIZE = 64;

        // List of all VPD (always nul-terminated)
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
