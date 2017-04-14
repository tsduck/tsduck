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
//
//  DirectShow & BDA utilities, Windows-specific.
//
//-----------------------------------------------------------------------------

#pragma once
#include "tsModulation.h"
#include "tsComPtr.h"

namespace ts {

    // Flags for pin selections. Bit masks allowed
    enum {
        xPIN_CONNECTED   = 0x01,
        xPIN_UNCONNECTED = 0x02,
        xPIN_INPUT       = 0x04,
        xPIN_OUTPUT      = 0x08,
        xPIN_ALL_INPUT   = xPIN_INPUT  | xPIN_CONNECTED | xPIN_UNCONNECTED,
        xPIN_ALL_OUTPUT  = xPIN_OUTPUT | xPIN_CONNECTED | xPIN_UNCONNECTED,
        xPIN_ALL         = xPIN_INPUT  | xPIN_OUTPUT    | xPIN_CONNECTED | xPIN_UNCONNECTED
    };

    typedef std::vector <ComPtr <::IPin>> PinPtrVector;

    // Get list of pins on a filter (use flags from enum above)
    // Return true on success, false on error.
    TSDUCKDLL bool GetPin (PinPtrVector&, ::IBaseFilter*, int flags, ReportInterface&);

    // Directly connect two filters using whatever output and input pin.
    // Return true on success, false on error.
    TSDUCKDLL bool ConnectFilters (::IGraphBuilder* graph,
                                 ::IBaseFilter* filter1,
                                 ::IBaseFilter* filter2,
                                 ReportInterface&);

    // In a DirectShow filter graph, cleanup everything downstream
    // a specified filter: all downstream filters are disconnected
    // and removed from the graph.
    // Return true on success, false on error.
    TSDUCKDLL bool CleanupDownstream (::IGraphBuilder*, ::IBaseFilter*, ReportInterface&);

    // Display the description of a DirectShow filter graph.
    // Start either at a specified filter (first definition)
    // or at one arbitray input filter (one with no connected
    // input pin) in a graph (second definition).
    // Return false on error.
    TSDUCKDLL bool DisplayFilterGraph (std::ostream&, const ComPtr <::IBaseFilter>&, const std::string& margin, bool verbose, ReportInterface&);
    TSDUCKDLL bool DisplayFilterGraph (std::ostream&, const ComPtr <::IGraphBuilder>&, const std::string& margin, bool verbose, ReportInterface&);

    // Display all devices of the specified category
    // Return false on error.
    TSDUCKDLL bool DisplayDevicesByCategory (std::ostream&, const ::GUID& category, const std::string& margin, const std::string& name, ReportInterface&);

    // Map a DirectShow network provider class id to a tuner type.
    // Return false if no match found.
    TSDUCKDLL bool NetworkProviderToTunerType (const ::GUID provider_clsid, TunerType& tuner_type);

    // Enumerate all devices of the specified class.
    // Fill a vector of monikers to these objects.
    // Return true on success, false on error.
    TSDUCKDLL bool EnumerateDevicesByClass (const ::CLSID& clsid, std::vector <ComPtr <::IMoniker>>& monikers, ReportInterface&);

    // Get names of a tuning space. Return empty string on error.
    TSDUCKDLL std::string GetTuningSpaceFriendlyName (::ITuningSpace*, ReportInterface&);
    TSDUCKDLL std::string GetTuningSpaceUniqueName (::ITuningSpace*, ReportInterface&);
}
