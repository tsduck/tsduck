//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------
//!
//!  @file
//!  Report options for the class TSAnalyzer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsjsonOutputArgs.h"

namespace ts {

    class Args;
    class DuckContext;

    //!
    //! Report options for the class TSAnalyzer.
    //! @ingroup mpeg
    //!
    //! The default options are
    //! -\-ts-analysis -\-service-analysis -\-pid-analysis -\-table-analysis
    //!
    class TSDUCKDLL TSAnalyzerOptions
    {
        TS_NOCOPY(TSAnalyzerOptions);
    public:
        //!
        //! Constructor.
        //!
        TSAnalyzerOptions();

        // Full analysis options:
        bool ts_analysis;            //!< Option -\-ts-analysis
        bool service_analysis;       //!< Option -\-service-analysis
        bool wide;                   //!< Option -\-wide-display
        bool pid_analysis;           //!< Option -\-pid-analysis
        bool table_analysis;         //!< Option -\-table-analysis
        bool error_analysis;         //!< Option -\-error-analysis

        // Normalized output:
        bool normalized;             //!< Option -\-normalized
        bool deterministic;          //!< Option -\-deterministic
        json::OutputArgs json;       //!< Options -\-json and -\-json-line

        // One-line report options:
        bool service_list;           //!< Option -\-service-list
        bool pid_list;               //!< Option -\-pid-list
        bool global_pid_list;        //!< Option -\-global-pid-list
        bool unreferenced_pid_list;  //!< Option -\-unreferenced-pid-list
        bool pes_pid_list;           //!< Option -\-pes-pid-list
        bool service_pid_list;       //!< Option -\-service-pid-list service-id
        uint16_t service_id;         //!< Service id for -\-service-pid-list
        UString prefix;              //!< Option -\-prefix "string"

        // Additional options
        UString title;               //!< Option -\-title "string"

        // Suspect packets detection
        uint64_t suspect_min_error_count;  //!< Option -\-suspect-min-error-count
        uint64_t suspect_max_consecutive;  //!< Option -\-suspect-max-consecutive

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(DuckContext& duck, Args& args);
    };
}
