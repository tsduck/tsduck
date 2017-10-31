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
//!  DirectShow & BDA utilities, Windows-specific.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsModulation.h"
#include "tsComPtr.h"

namespace ts {
    //!
    //! Flags for DirectShow filter pin selections (Windows-specific).
    //! Bit masks allowed.
    ///!
    enum DirectShowPinFilter {
        xPIN_CONNECTED   = 0x01,   //!< Filter connected pins.
        xPIN_UNCONNECTED = 0x02,   //!< Filter unconnected pins.
        xPIN_INPUT       = 0x04,   //!< Filter input pins.
        xPIN_OUTPUT      = 0x08,   //!< Filter output pins.
        xPIN_ALL_INPUT   = xPIN_INPUT  | xPIN_CONNECTED | xPIN_UNCONNECTED,   //!< Filter all input pins.
        xPIN_ALL_OUTPUT  = xPIN_OUTPUT | xPIN_CONNECTED | xPIN_UNCONNECTED,   //!< Filter all output pins.
        xPIN_ALL         = xPIN_INPUT  | xPIN_OUTPUT    | xPIN_CONNECTED | xPIN_UNCONNECTED   //!< Filter all pins.
    };

    //!
    //! Vector of COM pointers to IPin interfaces (Windows-specific).
    //!
    typedef std::vector<ComPtr <::IPin>> PinPtrVector;

    //!
    //! Get the list of pins on a DirectShow filter (Windows-specific).
    //! @param [out] pins Returned list of pins.
    //! @param [in,out] filter DirectShow filter.
    //! @param [in] flags Bit mask of pin selections from DirectShowPinFilter.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool GetPin(PinPtrVector& pins, ::IBaseFilter* filter, int flags, ReportInterface& report);

    //!
    //! Get the name for a DirectShow pin direction value (Windows-specific).
    //! @param [in] dir Pin direction.
    //! @return Corresponding name.
    //!
    TSDUCKDLL std::string PinDirectionName(::PIN_DIRECTION dir);

    //!
    //! Directly connect two DirectShow filters using whatever output and input pin (Windows-specific).
    //! @param [in,out] graph DirectShow graph builder.
    //! @param [in,out] filter1 DirectShow filter with output pins.
    //! @param [in,out] filter2 DirectShow filter with input pins.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool ConnectFilters(::IGraphBuilder* graph,
                                  ::IBaseFilter* filter1,
                                  ::IBaseFilter* filter2,
                                  ReportInterface& report);

    //!
    //! In a DirectShow filter graph, cleanup everything downstream a specified filter (Windows-specific).
    //! All downstream filters are disconnected and removed from the graph.
    //! @param [in,out] graph DirectShow graph builder.
    //! @param [in,out] filter DirectShow filter.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool CleanupDownstream(::IGraphBuilder* graph, ::IBaseFilter* filter, ReportInterface& report);

    //!
    //! Display the description of a DirectShow filter graph (Windows-specific).
    //! @param [in,out] strm Output text stream.
    //! @param [in] filter Start the graph description at this DirectShow filter.
    //! @param [in] margin Left margin to display.
    //! @param [in] verbose If true, display more verbose information.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool DisplayFilterGraph(std::ostream& strm,
                                      const ComPtr<::IBaseFilter>& filter,
                                      const std::string& margin,
                                      bool verbose,
                                      ReportInterface& report);

    //!
    //! Display the description of a DirectShow filter graph (Windows-specific).
    //! @param [in,out] strm Output text stream.
    //! @param [in] graph DirectShow graph builder. Start the graph description
    //! at one arbitray input filter (one with no connected input pin) in the graph.
    //! @param [in] margin Left margin to display.
    //! @param [in] verbose If true, display more verbose information.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool DisplayFilterGraph(std::ostream& strm,
                                      const ComPtr<::IGraphBuilder>& graph,
                                      const std::string& margin,
                                      bool verbose,
                                      ReportInterface& report);

    //!
    //! Display all devices of the specified category (Windows-specific).
    //! @param [in,out] strm Output text stream.
    //! @param [in] category Category of the devices to display.
    //! @param [in] margin Left margin to display.
    //! @param [in] name Name of the category to display.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool DisplayDevicesByCategory(std::ostream& strm,
                                            const ::GUID& category,
                                            const std::string& margin,
                                            const std::string& name,
                                            ReportInterface& report);

    //!
    //! Display all DirectShow tuning spaces (Windows-specific).
    //! @param [in,out] strm Output text stream.
    //! @param [in] margin Left margin to display.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool DisplayTuningSpaces(std::ostream& strm, const std::string& margin, ReportInterface& report);

    //!
    //! Translate a DirectShow network provider class id into a TSDuck tuner type (Windows-specific).
    //! @param [in] provider_clsid DirectShow network provider class.
    //! @param [out] tuner_type Returned TSDuck tuner type.
    //! @return True on success, false if no match is found.
    //!
    TSDUCKDLL bool NetworkProviderToTunerType(const ::GUID provider_clsid, TunerType& tuner_type);

    //!
    //! Enumerate all devices of the specified class.
    //! Fill a vector of monikers to these objects.
    //! @param [in] clsid Device class to enumerate.
    //! @param [out] monikers Returned vector of monikers to all devices of class @a clsid.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
   TSDUCKDLL bool EnumerateDevicesByClass(const ::CLSID& clsid,
                                           std::vector<ComPtr<::IMoniker>>& monikers,
                                           ReportInterface& report);

    //!
    //! Get the user-friendly name of a DirectShow tuning space (Windows-specific).
    //! @param [in] tuning Tuning space.
    //! @param [in,out] report Where to report errors.
    //! @return Tuning space name or an empty string on error.
    //!
    TSDUCKDLL std::string GetTuningSpaceFriendlyName(::ITuningSpace* tuning, ReportInterface& report);

    //!
    //! Get the unique name of a DirectShow tuning space (Windows-specific).
    //! @param [in] tuning Tuning space.
    //! @param [in,out] report Where to report errors.
    //! @return Tuning space name or an empty string on error.
    //!
    TSDUCKDLL std::string GetTuningSpaceUniqueName(::ITuningSpace* tuning, ReportInterface& report);

    //!
    //! Get the name for a DirectShow @c DVBSystemType value (Windows-specific).
    //! @param [in] type DVB system type value.
    //! @return Corresponding name.
    //!
    TSDUCKDLL std::string DVBSystemTypeName(::DVBSystemType type);
}
