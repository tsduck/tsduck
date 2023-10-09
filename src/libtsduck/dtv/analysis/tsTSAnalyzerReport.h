//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#include "tsNullReport.h"
#include "tsGrid.h"
#include "tsjson.h"

namespace ts {
    //!
    //! A subclass of TSAnalyzer with reporting capabilities.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TSAnalyzerReport: public TSAnalyzer
    {
        TS_NOBUILD_NOCOPY(TSAnalyzerReport);
    public:
        //!
        //! Default constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the analyzer.
        //! @param [in] bitrate_hint Optional bitrate "hint" for the analysis. It is the user-specified
        //! bitrate in bits/seconds, based on 188-byte packets. The bitrate hint is optional:
        //! if specified as zero, the analysis is based on the PCR values.
        //! @param [in] bitrate_confidence Confidence level in @a bitrate_hint.
        //!
        explicit TSAnalyzerReport(DuckContext& duck, const BitRate& bitrate_hint = 0, BitRateConfidence bitrate_confidence = BitRateConfidence::LOW);

        //!
        //! Virtual destructor.
        //!
        virtual ~TSAnalyzerReport() override;

        //!
        //! Set the analysis options.
        //! Must be set before feeding the first packet.
        //! @param [in] opt Analysis options.
        //!
        void setAnalysisOptions(const TSAnalyzerOptions& opt);

        //!
        //! General reporting method, using the specified options.
        //! @param [in,out] strm Output text stream.
        //! @param [in,out] opt Analysis options.
        //! @param [in,out] rep Where to report errors.
        //!
        void report(std::ostream& strm, TSAnalyzerOptions& opt, Report& rep = NULLREP);

        //!
        //! General reporting method, using the specified options.
        //! @param [in,out] opt Analysis options.
        //! @param [in,out] rep Where to report errors.
        //! @return The analysis as a String.
        //!
        UString reportToString(TSAnalyzerOptions& opt, Report& rep = NULLREP);

        //!
        //! Report formatted analysis about the global transport stream.
        //! @param [in,out] grid Output stream in a grid.
        //! @param [in] title Title string to display.
        //!
        void reportTS(Grid& grid, const UString& title = UString());

        //!
        //! Report formatted analysis about services.
        //! @param [in,out] grid Output stream in a grid.
        //! @param [in] title Title string to display.
        //!
        void reportServices(Grid& grid, const UString& title = UString());

        //!
        //! Report formatted analysis about PID's.
        //! @param [in,out] grid Output stream in a grid.
        //! @param [in] title Title string to display.
        //!
        void reportPIDs(Grid& grid, const UString& title = UString());

        //!
        //! Report formatted analysis about tables.
        //! @param [in,out] grid Output stream in a grid.
        //! @param [in] title Title string to display.
        //!
        void reportTables(Grid& grid, const UString& title = UString());

        //!
        //! This methods displays an error report.
        //! @param [in,out] strm Output text stream.
        //! @param [in] title Title string to display.
        //!
        void reportErrors(std::ostream& strm, const UString& title = UString());

        //!
        //! This methods displays a normalized report.
        //! @param [in,out] opt Analysis options.
        //! @param [in,out] strm Output text stream.
        //! @param [in] title Title string to display.
        //!
        void reportNormalized(TSAnalyzerOptions& opt, std::ostream& strm, const UString& title = UString());

        //!
        //! This methods displays a JSON report.
        //! @param [in,out] opt Analysis options.
        //! @param [in,out] strm Output text stream.
        //! @param [in] title Title string.
        //! @param [in,out] rep Where to report errors.
        //!
        void reportJSON(TSAnalyzerOptions& opt, std::ostream& strm, const UString& title = UString(), Report& rep = NULLREP);

    private:
        // Display header of a service PID list.
        void reportServiceHeader(Grid& grid, const UString& usage, bool scrambled, const BitRate& bitrate, const BitRate& ts_bitrate, bool wide) const;

         // Display one line of a subtotal.
         void reportServiceSubtotal(Grid& grid, const UString& header, const UString& usage, bool scrambled, const BitRate& bitrate, const BitRate& ts_bitrate, bool wide) const;

        // Display one line of a service PID list.
        void reportServicePID(Grid& grid, const PIDContext&) const;

        // Display list of services a PID belongs to.
        void reportServicesForPID(Grid& grid, const PIDContext&) const;

        // Report a time stamp.
        void reportTimeStamp(Grid& grid, const UString& name, const Time& value) const;

        // Display one normalized line of a time value.
        static void reportNormalizedTime(std::ostream&, const Time&, const char* type, const UString& country = UString());

        // Build a JSON time.
        static void jsonTime(json::Value& root, const UString& path, const Time&, const UString& country = UString());
    };
}
