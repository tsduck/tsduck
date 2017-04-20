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

    class TSDUCKDLL TSAnalyzerReport: public TSAnalyzer
    {
    public:
        // Constructor
        TSAnalyzerReport (BitRate bitrate_hint = 0) : TSAnalyzer (bitrate_hint) {}

        // Set analysis options. Must be set before feeding the first packet.
        void setAnalysisOptions (const TSAnalyzerOptions& opt);

        // General reporting method, using options
        std::ostream& report (std::ostream& strm, const TSAnalyzerOptions& opt);

        // The following methods display formatted analysis reports about various
        // aspects of the transport stream: global transport stream analysis,
        // services analysis, PID's analysis, tables.
        std::ostream& reportTS       (std::ostream& strm, const std::string& title = "");
        std::ostream& reportServices (std::ostream& strm, const std::string& title = "");
        std::ostream& reportPIDs     (std::ostream& strm, const std::string& title = "");
        std::ostream& reportTables   (std::ostream& strm, const std::string& title = "");

        // This methods displays an error report
        std::ostream& reportErrors (std::ostream& strm, const std::string& title = "");

        // This method displays a normalized report.
        std::ostream& reportNormalized (std::ostream& strm, const std::string& title = "");

    private:
        // Display one line of a service PID list 
        void reportServicePID (std::ostream&, const PIDContext&) const;

        // Display list of services a PID belongs to
        void reportServicesForPID (std::ostream&, const PIDContext&) const;

        // Display one normalized line of a time value
        static void reportNormalizedTime (std::ostream&, const Time&, const char* type, const std::string& country = "");
    };
}
