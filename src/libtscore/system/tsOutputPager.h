//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Send output to a "pager" application such as "more".
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsForkPipeOutputStream.h"
#include "tsUString.h"

namespace ts {
    //!
    //! Send application output to a "pager" application such as "more" or "less".
    //! Paging is done on stdout or stderr or both, depending on which is a terminal.
    //! If neither stdout nor stderr are terminals, paging is not allowed.
    //! @ingroup libtscore system
    //!
    class TSCOREDLL OutputPager: public ForkPipeOutputStream
    {
        TS_NOBUILD_NOCOPY(OutputPager);
    public:
        //!
        //! Default name of the environment variable containing the pager command.
        //! The default environment variable is @c PAGER.
        //!
        static constexpr const UChar* const DEFAULT_PAGER = u"PAGER";

        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] env_name Name of the optional environment variable containing the pager command name.
        //! @param [in] stdout_only If true, use only stdout. If false, if stdout is not a terminal but stderr is one, then use stderr for paging.
        //!
        explicit OutputPager(Report* report, const UString& env_name = DEFAULT_PAGER, bool stdout_only = false);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] env_name Name of the optional environment variable containing the pager command name.
        //! @param [in] stdout_only If true, use only stdout. If false, if stdout is not a terminal but stderr is one, then use stderr for paging.
        //!
        explicit OutputPager(ReporterBase* delegate, const UString& env_name = DEFAULT_PAGER, bool stdout_only = false);

        //!
        //! Destructor.
        //!
        virtual ~OutputPager() override;

        //!
        //! Check if we can run a pager.
        //! To run a pager, we must have found a valid pager command and either stdout or stderr must be a terminal.
        //! @return True if we can page.
        //!
        bool canPage() const { return _has_terminal && !_pager_command.empty(); }

        //!
        //! Get the pager command which is used.
        //! @return The pager command which is used.
        //!
        UString pagerCommand() const { return _pager_command; }

        //!
        //! Create the process, open the pipe.
        //! @param [in] synchronous If true, wait for process termination in close().
        //! @param [in] buffer_size The pipe buffer size in bytes. Used on Windows only. Zero means default.
        //! @return True on success, false on error.
        //!
        bool open(bool synchronous, size_t buffer_size);

        //!
        //! Write data to the pipe (received at process' standard input).
        //! @param [in] text Text to write.
        //! @return True on success, false on error.
        //!
        bool write(const UString& text);

    private:
        bool       _has_terminal = false;
        OutputMode _output_mode = KEEP_BOTH;
        UString    _pager_command {};

        // Constructors common code.
        void init(const UString& env_name, bool stdout_only);
    };
}
