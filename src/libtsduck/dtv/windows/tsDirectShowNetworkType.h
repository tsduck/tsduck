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
//!  Encapsulate a DirectShow network type and its properties.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsComPtr.h"
#include "tsDeliverySystem.h"
#include "tsDirectShow.h"

namespace ts {
    //!
    //! A class which encapsulates a DirectShow network type and its properties (Windows-specific).
    //! @ingroup windows
    //!
    class TSDUCKDLL DirectShowNetworkType
    {
    public:
        //!
        //! Constructor.
        //!
        DirectShowNetworkType();

        //!
        //! Destructor.
        //!
        ~DirectShowNetworkType();

        //!
        //! Clear all devices instances.
        //!
        void clear();

        //!
        //! Initialize this object from a network type.
        //! @param [in] network_type GUID of network type.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool initialize(const ::GUID& network_type, Report& report);

        //!
        //! Get the tuner type.
        //! @return The tuner type.
        //!
        TunerType tunerType() const { return _tuner_type; }

        //!
        //! Get delivery systems for that object.
        //! @param [in,out] sys A set of delivery systems where the delivery systems for this object are added.
        //!
        void getDeliverySystems(DeliverySystemSet& sys) const { sys.insert(_delivery_systems.begin(), _delivery_systems.end()); }

        //!
        //! Get the tuning space of this object.
        //! @return The tuning space of null on error.
        //!
        ::ITuningSpace* tuningSpace() const { return _tuning_space.pointer(); }

        //!
        //! Get the tuning space name of this object.
        //! @return The tuning space name.
        //!
        UString tuningSpaceName() const { return _tuning_space_name; }

    private:
        ::GUID                 _network_type;
        UString                _network_type_name;
        ::DVBSystemType        _system_type;
        ::TunerInputType       _input_type;
        TunerType              _tuner_type;
        DeliverySystemSet      _delivery_systems;
        ComPtr<::ITuningSpace> _tuning_space;
        UString                _tuning_space_name;

        // In header file 'tuner.h', the Windows 10 kit defines the following
        // classes and interfaces:
        //
        //   CLSID_DVBTuningSpace           IID_IDVBTuningSpace      IID_IDVBTuningSpace2
        //   CLSID_DVBSTuningSpace          IID_IDVBSTuningSpace
        //   CLSID_ATSCTuningSpace          IID_IATSCTuningSpace
        //   CLSID_DigitalCableTuningSpace  IID_IDigitalCableTuningSpace
        //
        //   CLSID_DigitalLocator           IID_IDigitalLocator
        //   CLSID_DVBCLocator              IID_IDVBCLocator
        //   CLSID_DVBTLocator              IID_IDVBTLocator
        //   CLSID_DVBTLocator2             IID_IDVBTLocator2
        //   CLSID_DVBSLocator              IID_IDVBSLocator         IID_IDVBSLocator2
        //   CLSID_ISDBSLocator             IID_IISDBSLocator
        //   CLSID_ATSCLocator              IID_IATSCLocator         IID_IATSCLocator2
        //   CLSID_DigitalCableLocator      IID_IDigitalCableLocator
        //
        //   CLSID_DVBTuneRequest           IID_IDVBTuneRequest
        //   CLSID_DigitalCableTuneRequest  IID_IDigitalCableTuneRequest
        //   CLSID_ATSCChannelTuneRequest   IID_IATSCChannelTuneRequest
        //   CLSID_ATSCComponentType        IID_IATSCComponentType
        //
        // Tuning space class hierarchy:
        //
        // ITuningSpace -+--> IDVBTuningSpace --> IDVBTuningSpace2 --> IDVBSTuningSpace
        //               |
        //               +--> IAnalogTVTuningSpace --> IATSCTuningSpace --> IDigitalCableTuningSpace
        //
        // Locator class hierarchy:
        //
        // ILocator --> IDigitalLocator -+--> IDVBTLocator ----> IDVBTLocator2
        //                               |
        //                               +--> IDVBSLocator -+--> IDVBSLocator2
        //                               |                  |
        //                               |                  +--> IISDBSLocator
        //                               |
        //                               +--> IDVBCLocator
        //                               |
        //                               +--> IATSCLocator --> IATSCLocator2 --> IDigitalCableLocator
        //

        // Initialize the content of a TuningSpace object with a name and a default locator.
        bool initTuningSpace(::ITuningSpace* tspace, const ::WCHAR* name, ::ILocator* dlocator, Report& report);
        bool initDVBTuningSpace(::IDVBTuningSpace* tspace, const ::WCHAR* name, ::ILocator* dlocator, Report& report);
        bool initDVBTuningSpace2(::IDVBTuningSpace2* tspace, const ::WCHAR* name, ::ILocator* dlocator, Report& report);
        bool initDVBSTuningSpace(::IDVBSTuningSpace* tspace, const ::WCHAR* name, ::ILocator* dlocator, Report& report);
        bool initATSCTuningSpace(::IATSCTuningSpace* tspace, const ::WCHAR* name, ::ILocator* dlocator, Report& report);
        bool initDigitalCableTuningSpace(::IDigitalCableTuningSpace* tspace, const ::WCHAR* name, ::ILocator* dlocator, Report& report);

        // It has been noticed that the default locator shall be set on a tuning space after
        // all other settings. This is specifically required for DVB tuning spaces. Initialization
        // functions for subclasses of the tuning space always pass null to delay the setting of
        // the default locator after all parameters are set. The following method sets the default
        // locator, ignoring null pointers.
        bool initDefaultLocator(::ITuningSpace* tspace, ::ILocator* dlocator, Report& report);

        // Initialize the content of a Locator object.
        bool initDigitalLocator(::IDigitalLocator* locator, Report& report);
        bool initDVBCLocator(::IDVBCLocator* locator, Report& report);
        bool initDVBTLocator(::IDVBTLocator* locator, Report& report);
        bool initDVBTLocator2(::IDVBTLocator2* locator, Report& report);
        bool initDVBSLocator(::IDVBSLocator* locator, Report& report);
        bool initDVBSLocator2(::IDVBSLocator2* locator, Report& report);
        bool initISDBSLocator(::IISDBSLocator* locator, Report& report);
        bool initATSCLocator(::IATSCLocator* locator, Report& report);
        bool initATSCLocator2(::IATSCLocator2* locator, Report& report);
        bool initDigitalCableLocator(::IDigitalCableLocator* locator, Report& report);
    };
}
