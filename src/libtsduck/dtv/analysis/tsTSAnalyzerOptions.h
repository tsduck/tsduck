//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        TSAnalyzerOptions() = default;

        // Full analysis options:
        bool ts_analysis = false;            //!< Option -\-ts-analysis
        bool service_analysis = false;       //!< Option -\-service-analysis
        bool wide = false;                   //!< Option -\-wide-display
        bool pid_analysis = false;           //!< Option -\-pid-analysis
        bool table_analysis = false;         //!< Option -\-table-analysis
        bool error_analysis = false;         //!< Option -\-error-analysis

        // Normalized output:
        bool normalized = false;             //!< Option -\-normalized
        bool deterministic = false;          //!< Option -\-deterministic
        json::OutputArgs json {};            //!< Options -\-json and -\-json-line

        // One-line report options:
        bool service_list = false;           //!< Option -\-service-list
        bool pid_list = false;               //!< Option -\-pid-list
        bool global_pid_list = false;        //!< Option -\-global-pid-list
        bool unreferenced_pid_list = false;  //!< Option -\-unreferenced-pid-list
        bool pes_pid_list = false;           //!< Option -\-pes-pid-list
        bool service_pid_list = false;       //!< Option -\-service-pid-list service-id
        uint16_t service_id = 0;             //!< Service id for -\-service-pid-list
        UString prefix {};                   //!< Option -\-prefix "string"

        // Additional options
        UString title {};                    //!< Option -\-title "string"

        // Suspect packets detection
        uint64_t suspect_min_error_count = 1;  //!< Option -\-suspect-min-error-count
        uint64_t suspect_max_consecutive = 1;  //!< Option -\-suspect-max-consecutive

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
