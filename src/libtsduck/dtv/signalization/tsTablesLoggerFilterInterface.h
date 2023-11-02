//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  An interface which is used to filter sections in TablesLogger.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSafePtr.h"
#include "tsTS.h"

namespace ts {

    class DuckContext;
    class Section;
    class Args;

    //!
    //! An interface which is used to filter sections in TablesLogger.
    //! @ingroup mpeg
    //!
    //! This abstract interface must be implemented by classes which
    //! define filtering rules for TablesLogger. There is one main
    //! instance which comes from TSDuck. Additional instances may
    //! be defined by external extensions.
    //!
    class TSDUCKDLL TablesLoggerFilterInterface
    {
        TS_INTERFACE(TablesLoggerFilterInterface);
    public:
        //!
        //! Define section filtering command line options in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        virtual void defineFilterOptions(Args& args) const = 0;

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] duck TSDuck context.
        //! @param [in,out] args Command line arguments.
        //! @param [out] initial_pids Initial PID's that the filter would like to see.
        //! @return True on success, false on error in argument line.
        //!
        virtual bool loadFilterOptions(DuckContext& duck, Args& args, PIDSet& initial_pids) = 0;

        //!
        //! Reset context, if filtering restarts from the beginning for instance.
        //! @return True on success, false on error in argument line.
        //!
        virtual bool reset() = 0;

        //!
        //! Check if a specific section must be filtered and displayed.
        //! @param [in,out] duck TSDuck context.
        //! @param [in] section The section to check.
        //! @param [in] cas The CAS id for this section.
        //! @param [out] more_pids Additional PID's that the filter would like to see.
        //! @return True if the section can be displayed, false if it must not be displayed.
        //! A section is actually displayed if all section filters returned true.
        //!
        virtual bool filterSection(DuckContext& duck, const Section& section, uint16_t cas, PIDSet& more_pids) = 0;
    };

    //!
    //! A safe pointer to TablesLogger section filter (not thread-safe).
    //!
    typedef SafePtr<TablesLoggerFilterInterface> TablesLoggerFilterPtr;

    //!
    //! A vector of safe pointers to TablesLogger section filters.
    //!
    typedef std::vector<TablesLoggerFilterPtr> TablesLoggerFilterVector;
}
