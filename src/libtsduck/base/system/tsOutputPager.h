//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Send output to a "pager" application such as "more".
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsForkPipe.h"
#include "tsUString.h"

namespace ts {
    //!
    //! Send application output to a "pager" application such as "more" or "less".
    //! Paging is done on stdout or stderr or both, depending on which is a terminal.
    //! If neither stdout nor stderr are terminals, paging is not allowed.
    //! @ingroup system
    //!
    class TSDUCKDLL OutputPager : public ForkPipe
    {
        TS_NOCOPY(OutputPager);
    public:
        //!
        //! Default name of the environment variable containing the pager command.
        //! The default environment variable is @c PAGER.
        //!
        static const UChar* const DEFAULT_PAGER;

        //!
        //! Default constructor.
        //! @param [in] envName Name of the optional environment variable containing the pager command name.
        //! @param [in] stdoutOnly If true, use only stdout. If false, if stdout is not a terminal but stderr
        //! is one, then use stderr for paging.
        //!
        explicit OutputPager(const UString& envName = DEFAULT_PAGER, bool stdoutOnly = false);

        //!
        //! Destructor.
        //!
        virtual ~OutputPager() override;

        //!
        //! Check if we can run a pager.
        //! To run a pager, we must have found a valid pager command and either stdout or stderr must be a terminal.
        //! @return True if we can page.
        //!
        bool canPage() const { return _hasTerminal && !_pagerCommand.empty(); }

        //!
        //! Get the pager command which is used.
        //! @return The pager command which is used.
        //!
        UString pagerCommand() const { return _pagerCommand; }

        //!
        //! Create the process, open the pipe.
        //! @param [in] synchronous If true, wait for process termination in close().
        //! @param [in] buffer_size The pipe buffer size in bytes. Used on Windows only. Zero means default.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool open(bool synchronous, size_t buffer_size, Report& report);

        //!
        //! Write data to the pipe (received at process' standard input).
        //! @param [in] text Text to write.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool write(const UString& text, Report& report);

    private:
        bool       _hasTerminal = false;
        OutputMode _outputMode {KEEP_BOTH};
        UString    _pagerCommand {};
    };
}
