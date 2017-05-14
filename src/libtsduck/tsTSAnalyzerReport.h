//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------
//!
//!  @file
//!  A subclass of TSAnalyzer with reporting capabilities
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSAnalyzer.h"
#include "tsTSAnalyzerOptions.h"

namespace ts {
    //!
    //! A subclass of TSAnalyzer with reporting capabilities.
    //!
    class TSDUCKDLL TSAnalyzerReport: public TSAnalyzer
    {
    public:
        //!
        //! Default constructor.
        //! @param [in] bitrate_hint Optional bitrate "hint" for the analysis.
        //! It is the user-specified bitrate in bits/seconds, based on 188-byte
        //! packets. The bitrate hint is optional: if specified as zero, the
        //! analysis is based on the PCR values.
        //!
        TSAnalyzerReport(BitRate bitrate_hint = 0) :
            TSAnalyzer(bitrate_hint)
        {
        }

        //!
        //! Set the analysis options.
        //! Must be set before feeding the first packet.
        //! @param [in] opt Analysis options.
        //!
        void setAnalysisOptions(const TSAnalyzerOptions& opt);

        //!
        //! General reporting method, using the specified options.
        //! @param [in,out] strm Output text stream.
        //! @param [in] opt Analysis options.
        //! @return A reference to @a strm.
        //!
        std::ostream& report(std::ostream& strm, const TSAnalyzerOptions& opt);

        //!
        //! Report formatted analysis about the global transport stream.
        //! @param [in,out] strm Output text stream.
        //! @param [in] title Title string to display.
        //! @return A reference to @a strm.
        //!
        std::ostream& reportTS(std::ostream& strm, const std::string& title = "");

        //!
        //! Report formatted analysis about services.
        //! @param [in,out] strm Output text stream.
        //! @param [in] title Title string to display.
        //! @return A reference to @a strm.
        //!
        std::ostream& reportServices(std::ostream& strm, const std::string& title = "");

        //!
        //! Report formatted analysis about PID's.
        //! @param [in,out] strm Output text stream.
        //! @param [in] title Title string to display.
        //! @return A reference to @a strm.
        //!
        std::ostream& reportPIDs(std::ostream& strm, const std::string& title = "");

        //!
        //! Report formatted analysis about tables.
        //! @param [in,out] strm Output text stream.
        //! @param [in] title Title string to display.
        //! @return A reference to @a strm.
        //!
        std::ostream& reportTables(std::ostream& strm, const std::string& title = "");

        //!
        //! This methods displays an error report.
        //! @param [in,out] strm Output text stream.
        //! @param [in] title Title string to display.
        //! @return A reference to @a strm.
        //!
        std::ostream& reportErrors(std::ostream& strm, const std::string& title = "");

        //!
        //! This methods displays a normalized report.
        //! @param [in,out] strm Output text stream.
        //! @param [in] title Title string to display.
        //! @return A reference to @a strm.
        //!
        std::ostream& reportNormalized(std::ostream& strm, const std::string& title = "");

    private:
        // Display one line of a service PID list 
        void reportServicePID(std::ostream&, const PIDContext&) const;

        // Display list of services a PID belongs to
        void reportServicesForPID(std::ostream&, const PIDContext&) const;

        // Display one normalized line of a time value
        static void reportNormalizedTime(std::ostream&, const Time&, const char* type, const std::string& country = "");
    };
}
