//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  An encapsulation of a HiDes modulator device.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsModulation.h"
#include "tsCerrReport.h"

namespace ts {
    //!
    //! Encapsulation of a HiDes modulator device.
    //! @ingroup hardware
    //!
    class TSDUCKDLL HiDesDevice
    {
    public:
        //!
        //! Constructor.
        //!
        HiDesDevice();

        //!
        //! Destructor.
        //!
        virtual ~HiDesDevice();

        //!
        //! Information about a device.
        //!
        class TSDUCKDLL Info
        {
        public:
            int     index;   //!< Adapter index.
            UString name;    //!< Device name.

            //!
            //! Default constructor.
            //!
            Info() : index(0), name() {}
        };

        //!
        //! A list of device information.
        //!
        typedef std::list<Info> InfoList;

        //!
        //! Get all HiDes devices in the system.
        //! @param [out] devices Returned list of devices.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        static bool GetAllDevices(InfoList& devices, Report& report = CERR);

    private:
        // The implementation is highly system-dependent.
        // Redirect it to an internal "guts" class.
        class Guts;
        Guts* _guts;

        // Inaccessible operations.
        HiDesDevice(const HiDesDevice&) = delete;
        HiDesDevice& operator=(const HiDesDevice&) = delete;
    };
}
