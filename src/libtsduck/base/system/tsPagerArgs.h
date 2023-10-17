//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command line arguments for --pager or --no-pager.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsOutputPager.h"
#include "tsCerrReport.h"

namespace ts {

    class Args;
    class DuckContext;

    //!
    //! Command line arguments for @c -\-pager or @c -\-no-pager.
    //! @ingroup cmd
    //!
    class TSDUCKDLL PagerArgs
    {
        TS_NOCOPY(PagerArgs);
    public:
        // Public fields
        bool page_by_default = false; //!< Use a page process by default.
        bool use_pager = false;       //!< Actually use a page process.

        //!
        //! Default constructor.
        //! @param [in] pageByDefault If true, paging is enabled by default and option @c -\-no-pager
        //! is defined. If false, do not page by default and option @c -\-pager is defined.
        //! @param [in] stdoutOnly If true, use only stdout. If false, if stdout is not a terminal but
        //! stderr is one, then use stderr for paging.
        //!
        PagerArgs(bool pageByDefault = false, bool stdoutOnly = true);

        //!
        //! Destructor.
        //!
        ~PagerArgs();

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

        //!
        //! Return the output device for display.
        //! @param [in,out] report Where to report errors.
        //! @return A reference to the output device, either @c std::cout or a pager stream.
        //!
        std::ostream& output(Report& report = CERR);

    private:
        OutputPager _pager;
    };
}
