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
//!  An encapsulation of a DirectShow graph.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsComPtr.h"
#include "tsDirectShow.h"

namespace ts {
    //!
    //! An encapsulation of a DirectShow graph (Windows-specific).
    //! @ingroup windows
    //!
    class TSDUCKDLL DirectShowGraph
    {
        TS_NOCOPY(DirectShowGraph);
    public:
        //!
        //! Default constructor.
        //!
        DirectShowGraph();

        //!
        //! Destructor.
        //!
        virtual ~DirectShowGraph();

        //!
        //! Initialize the graph.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool initialize(Report& report);

        //!
        //! Clear the graph back to uninitialized state.
        //! @param [in,out] report Where to report errors.
        //!
        virtual void clear(Report& report);

        //!
        //! Check if the graph was correctly initialized.
        //! @return True if the graph was correctly initialized.
        //!
        bool isValid() const;

        //!
        //! Add a filter in the graph.
        //! @param [in,out] filter The filter to add.
        //! @param [in] name Filter name.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool addFilter(::IBaseFilter* filter, const wchar_t* name, Report& report);

        //!
        //! Remove a filter from the graph.
        //! @param [in,out] filter The filter to remove.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool removeFilter(::IBaseFilter* filter, Report& report);

        //!
        //! Directly connect two filters using whatever output and input pin.
        //! @param [in,out] filter1 DirectShow filter with output pins.
        //! @param [in,out] filter2 DirectShow filter with input pins.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool connectFilters(::IBaseFilter* filter1, ::IBaseFilter* filter2, Report& report);

        //!
        //! In the graph, cleanup everything downstream a specified filter.
        //! All downstream filters are disconnected and removed from the graph.
        //! @param [in,out] filter DirectShow filter.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool cleanupDownstream(::IBaseFilter* filter, Report& report);

        //!
        //! Get the starting filter of the graph.
        //! @param [in,out] report Where to report errors.
        //! @return A pointer to the first filter with no connected input pin or a null pointer if not found.
        //!
        ComPtr<::IBaseFilter> startingFilter(Report& report);

        //!
        //! Run the graph.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool run(Report& report);

        //!
        //! Stop the graph.
        //! @param [in,out] report Where to report errors.
        //! @return True on success or already stopped, false on error.
        //!
        bool stop(Report& report);

        //!
        //! Display the description of the graph.
        //! @param [in,out] output Where to display test results.
        //! @param [in,out] report Where to report errors.
        //! @param [in] margin Left margin to display.
        //! @param [in] verbose True for verbose information.
        //!
        void display(std::ostream& output, Report& report, const UString& margin = UString(), bool verbose = true);

    private:
        ComPtr<::IGraphBuilder> _graph_builder;
        ComPtr<::IMediaControl> _media_control;

        //!
        //! Flags for DirectShow filter pin selections, bit masks allowed.
        //!
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
        //! Vector of COM pointers to IPin interfaces.
        //!
        typedef std::vector<ComPtr<::IPin>> PinPtrVector;

        //!
        //! Get the list of pins on a filter.
        //! @param [out] pins Returned list of pins.
        //! @param [in,out] filter DirectShow filter.
        //! @param [in] flags Bit mask of pin selections from DirectShowPinFilter.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool getPin(PinPtrVector& pins, ::IBaseFilter* filter, int flags, Report& report);

        //!
        //! Display the description of a partial graph.
        //! @param [in,out] output Where to display test results.
        //! @param [in,out] report Where to report errors.
        //! @param [in] filter Start the graph description at this filter.
        //! @param [in] margin Left margin to display.
        //! @param [in] verbose True for verbose information.
        //!
        void display(std::ostream& output, Report& report, const ComPtr<::IBaseFilter>& filter, const UString& margin, bool verbose);
    };
}
