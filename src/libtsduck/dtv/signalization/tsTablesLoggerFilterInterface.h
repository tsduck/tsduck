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
        //! Check if a specific section must be filtered and displayed.
        //! @param [in,out] duck TSDuck context.
        //! @param [in] section The section to check.
        //! @param [in] cas The CAS id for this section.
        //! @param [out] more_pids Additional PID's that the filter would like to see.
        //! @return True if the section can be displayed, false if it must not be displayed.
        //! A section is actually displayed if all section filters returned true.
        //!
        virtual bool filterSection(DuckContext& duck, const Section& section, uint16_t cas, PIDSet& more_pids) = 0;

        //!
        //! Virtual destructor.
        //!
        virtual ~TablesLoggerFilterInterface();
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
