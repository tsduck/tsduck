//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  Perform tests on DirectShow & BDA, Windows-specific.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsEnumeration.h"
#include "tsReport.h"
#include "tsComPtr.h"
#include "tsDirectShow.h"

namespace ts {
    //!
    //! A class to perform various tests on DirectShow and BDA (Windows-specific).
    //! @ingroup windows
    //!
    class TSDUCKDLL DirectShowTest
    {
        TS_NOBUILD_NOCOPY(DirectShowTest);
    public:
        //!
        //! Constructor.
        //! @param [in,out] output Where to display test results.
        //! @param [in,out] report Where to report errors.
        //!
        DirectShowTest(std::ostream& output, Report& report);

        //!
        //! List of DirectShow tests.
        //!
        enum TestType {
            NONE,                //!< No test, do nothing.
            LIST_DEVICES,        //!< Briel list of most tuner-related DirectShow devices.
            ENUMERATE_DEVICES,   //!< Enumerate all tuner-related DirectShow devices.
            TUNING_SPACES,       //!< Test available tuning spaces.
            BDA_TUNERS,          //!< Test available BDA tuners.
        };

        //!
        //! An enumeration of TestType names, typically used for command line options.
        //!
        static const Enumeration TestNames;

        //!
        //! Run one test.
        //! @param [in] type The test to run.
        //!
        void runTest(TestType type);

        //!
        //! Brief list of DirectShow devices, same as runTest(LIST_DEVICES).
        //! @param [in] margin Left margin to display.
        //!
        void listDevices(const UString& margin = UString());

        //!
        //! Enumerate DirectShow devices, same as runTest(ENUMERATE_DEVICES).
        //! @param [in] margin Left margin to display.
        //!
        void enumerateDevices(const UString& margin = UString());

        //!
        //! Test tuning spaces, same as runTest(TUNING_SPACES).
        //! @param [in] margin Left margin to display.
        //!
        void testTuningSpaces(const UString& margin = UString());

        //!
        //! Test BDA tuners, same as runTest(BDA_TUNERS).
        //! @param [in] margin Left margin to display.
        //!
        void testBDATuners(const UString& margin = UString());

        //!
        //! Display all devices of the specified category.
        //! @param [in] category Category of the devices to display.
        //! @param [in] name Name of the category to display.
        //! @param [in] details When true, list all details.
        //! @param [in] margin Left margin to display.
        //! @return True on success, false on error.
        //!
        bool displayDevicesByCategory(const ::GUID& category, const UString& name, bool details = true, const UString& margin = UString());

        //!
        //! Display all DirectShow tuning spaces.
        //! @param [in] margin Left margin to display.
        //! @return True on success, false on error.
        //!
        bool displayTuningSpaces(const UString& margin = UString());

        //!
        //! Show selected properties of a COM object.
        //! @param [in] object Object to query.
        //! @param [in] margin Left margin to display.
        //!
        void displayObject(::IUnknown* object, const UString& margin = UString());

        //!
        //! List some known interfaces that an object may expose.
        //! Warning, very slow, test interfaces one by one.
        //! @param [in] object Object to query.
        //! @param [in] margin Left margin to display.
        //!
        void displayInterfaces(::IUnknown* object, const UString& margin = UString());

    private:
        std::ostream&    _output;
        Report& _report;

        // Get an enumerator for all tuning spaces.
        bool getAllTuningSpaces(ComPtr<::ITuningSpaceContainer>& tsContainer, ComPtr<::IEnumTuningSpaces>& tsEnum);

        // Get all tuning spaces.
        bool getAllTuningSpaces(std::vector<ComPtr<::ITuningSpace>>& spaces);

        // Display all tuning spaces from an enumerator.
        void displayEnumerateTuningSpaces(::IEnumTuningSpaces* enum_tspace, const UString& margin = UString());

        // Show ITuner for a COM object
        void displayITuner(::IUnknown* object, const UString& margin = UString());

        // Show IKsTopologyInfo for a COM object
        void displayIKsTopologyInfo(::IUnknown* object, const UString& margin = UString());

        // Show IBDA_Topology for a COM object
        void displayBDATopology(::IUnknown* object, const UString& margin = UString());

        // Show properties support through IKsPropertySet for a COM object.
        void displayIKsPropertySet(::IUnknown* object, const UString& margin = UString());

        // Show one property support through IKsPropertySet for a COM object.
        void displayOneIKsPropertySet(::IKsPropertySet* ps, const ::GUID& psGuid, const char* psName, ::DWORD propId, const char* propName, const UString& margin = UString());

        // Show properties support through IKsControl for a COM object.
        void displayIKsControl(::IUnknown* object, const UString& margin = UString());

        // Show one properties support through IKsControl for a COM object.
        void displayOneIKsControl(::IKsControl* iks, const ::GUID& propSetGuid, const char* propSetName, ::ULONG propId, const char* propName, const UString& margin = UString());
    };
}
