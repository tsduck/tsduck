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
//!  @file tsDektecDevice.h
//!
//!  Declare the class ts::DektecDevice.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsDektec.h"
#include "tsCerrReport.h"
#include "tsEnumeration.h"

namespace ts {

#if !defined(TS_NO_DTAPI)
                                       
    class DektecDevice;
    typedef std::vector<DektecDevice> DektecDeviceVector;
    typedef std::vector<Dtapi::DtDeviceDesc> DektecDeviceDescVector;
    typedef std::vector<Dtapi::DtHwFuncDesc> DektecPortDescVector;

    //!
    //! Get the error message corresponding to a DTAPI error code
    //! @return An error message.
    //!
    std::string DektecStrError(Dtapi::DTAPI_RESULT);

    //!
    //! Description of a Dektec device.
    //!
    class DektecDevice
    {
    public:
        std::string          model;    //!< Device model name.
        Dtapi::DtDeviceDesc  desc;     //!< Device description, as returned by DTAPI.
        DektecPortDescVector input;    //!< Vector of input ports.
        DektecPortDescVector output;   //!< Vector of output ports.

        // Constructor
        DektecDevice() : model(), desc(), input(), output() {}

        // Get a Dektec device description. Return true on success.
        // If dev_index or chan_index are negative, update them.
        bool getDevice(int& dev_index, int& chan_index, bool is_input, ReportInterface& report = CERR);

        // Get the list of all Dektec devices in the system.
        // Return true in case of success, false on error.
        // Report error messages through report
        static bool GetAllDevices(DektecDeviceVector&, ReportInterface& = CERR);

        // Get the list of all Dektec ports in the system.
        // If is_input and/or is_output are true, return only the ports which are currently in the right direction.
        // If is_bidirectional is true, also report bidirectional ASI ports which are currently not in the right direction.
        // TS-over-IP ports are always considered as both input and output ports. Remote network devices (DTE-xxxx) are not returned.
        // Return true in case of success, false on error. Report error messages through report
        static bool GetAllPorts(DektecPortDescVector&, bool is_input, bool is_output, bool is_bidirectional, ReportInterface& = CERR);

        // Get a string description of a Dektec device or port.
        static std::string GetDeviceDescription(const Dtapi::DtDeviceDesc&);
        static std::string GetPortDescription(const Dtapi::DtHwFuncDesc&);

        // Get a string description of a Dektec interface type
        static std::string GetInterfaceDescription(const Dtapi::DtHwFuncDesc&);

        // Display various Dektec data structure for debug
        static void Report(const Dtapi::DtDvbT2Pars&, ReportInterface& = CERR, int severity = Severity::Info, const std::string& margin = "");
        static void Report(const Dtapi::DtDvbT2PlpPars&, ReportInterface& = CERR, int severity = Severity::Info, const std::string& margin = "");
        static void Report(const Dtapi::DtDvbT2ParamInfo&, ReportInterface& = CERR, int severity = Severity::Info, const std::string& margin = "");

    private:
        // Append a name to a string if a condition is true.
        static void OneCap(std::string& str, bool condition, const std::string& name);
        static void OneCap(std::string& str, Dtapi::DtCaps cap, const std::string& name);
    };
    
#endif // TS_NO_DTAPI
}
