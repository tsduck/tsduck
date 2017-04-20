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
//!  Report options for the class TSAnalyzer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"

namespace ts {

    class TSDUCKDLL TSAnalyzerOptions: public Args
    {
    public:
        // Constructor.
        TSAnalyzerOptions (const std::string& description = "",
                           const std::string& syntax = "",
                           const std::string& help = "",
                           int flags = 0);

        // Public fields, by options. The default options are
        // --ts-analysis --service-analysis --pid-analysis --table-analysis

        // Full analysis options:
        bool ts_analysis;            // --ts-analysis
        bool service_analysis;       // --service-analysis
        bool pid_analysis;           // --pid-analysis
        bool table_analysis;         // --table-analysis
        bool error_analysis;         // --error-analysis

        // Normalized output:
        bool normalized;             // --normalized

        // One-line report options:
        bool service_list;           // --service-list
        bool pid_list;               // --pid-list
        bool global_pid_list;        // --global-pid-list
        bool unreferenced_pid_list;  // --unreferenced-pid-list
        bool pes_pid_list;           // --pes-pid-list
        bool service_pid_list;       // --service-pid-list service-id
        uint16_t service_id;           //
        std::string prefix;          // --prefix "string"

        // Additional options
        std::string title;           // --title "string"

        // Suspect packets detection
        uint64_t suspect_min_error_count;  // --suspect-min-error-count
        uint64_t suspect_max_consecutive;  // --suspect-max-consecutive

        // Overriden methods.
        void setHelp (const std::string& help);
        virtual bool analyze (int argc, char* argv[]);
        virtual bool analyze (const std::string& app_name, const StringVector& arguments);

        // Get option values (the public fields) after analysis of another
        // ts::Args object defining the same options.
        void getOptions (const Args&);

    private:
        // Inaccessible operations
        virtual bool analyze (const char* app_name, const char* arg1, ...);
    };
}
