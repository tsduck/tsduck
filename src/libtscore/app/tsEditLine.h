//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Read input lines with bash-like line editing when possible.
//!
//!  This class interfaces "libedit" when available. The classical "readline"
//!  library is not used because of its radical GPL v3 license (not LGPL)
//!  which is incompatible with the BSD license of the present code. The
//!  alternative library "libedit" was specially designed with a BSD license.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! Read input lines with bash-like line editing when possible.
    //! @ingroup libtscore app
    //!
    class TSCOREDLL EditLine
    {
    public:
        //!
        //! Get the default history file name.
        //! The default initial file location depends on the operating system:
        //! - Windows: @c \%APPDATA%\\tsduck\\.tshistory
        //! - Unix: @c $HOME/.tshistory
        //! @return The default history file name.
        //!
        static UString DefaultHistoryFile() { return _default_history_file; }

        //!
        //! Set the default history file name.
        //! @param [in] history_file The default history file name.
        //!
        static void SetDefaultHistoryFile(const UString& history_file) { _default_history_file = history_file; }

        //!
        //! Get the default maximum number of history lines to save.
        //! The initial default is 100 lines.
        //! @return The default maximum number of history lines to save.
        //!
        static size_t DefaultHistorySize() { return _default_history_size; }

        //!
        //! Set the default maximum number of history lines to save.
        //! @param [in] history_size The default maximum number of history lines to save.
        //!
        static void SetDefaultHistorySize(size_t history_size) { _default_history_size = history_size; }

        //!
        //! Get the default command line prompt.
        //! The initial default is "> ".
        //! @return The default command line prompt.
        //!
        static UString DefaultPrompt() { return _default_prompt; }

        //!
        //! Set the default command line prompt.
        //! @param [in] prompt Command line prompt.
        //!
        static void setDefaultPrompt(const UString& prompt) { _default_prompt = prompt; }

        //!
        //! Get the default command line prompt for continuation lines (after a backslash).
        //! The initial default is ">>> ".
        //! @return The default command line prompt for continuation lines.
        //!
        static UString DefaultNextPrompt() { return _default_next_prompt; }

        //!
        //! Set the default command line prompt for continuation lines (after a backslash).
        //! @param [in] prompt Command line prompt for continuation lines.
        //!
        static void setDefaultNextPrompt(const UString& prompt) { _default_next_prompt = prompt; }

        //!
        //! Constructor.
        //! @param [in] prompt Command line prompt.
        //! @param [in] next_prompt Command line prompt.
        //! @param [in] history_file File to load/save the history.
        //! The history is loaded in the constructor and saved in the destructor.
        //! If empty, no history is loaded.
        //! @param [in] history_size Maximum number of history lines to save.
        //!
        EditLine(const UString& prompt = DefaultPrompt(),
                 const UString& next_prompt = DefaultNextPrompt(),
                 const UString& history_file = DefaultHistoryFile(),
                 size_t history_size = DefaultHistorySize());

        //!
        //! Destructor.
        //!
        ~EditLine();

        //!
        //! Read one line of input.
        //! @param [out] line Returned line.
        //! @param [in] skip_empty Skip empty lines, continue reading until a non-empty line is read.
        //! @param [in] trim Trim leading and trailing spaces.
        //! @param [in] continuing Continue reading lines when the end of line is a backslash and return a full rebuilt line.
        //! @return True on success, false on error (typically end of input).
        //!
        bool readLine(UString& line, bool skip_empty = true, bool trim = true, bool continuing = true);

    private:
        bool    _is_a_tty = false;
        bool    _end_of_file = false;
        UString _prompt {};
        UString _next_prompt {};
        UString _previous_line {};
        [[maybe_unused]] bool    _update_history = false;
        [[maybe_unused]] UString _history_file {};
        [[maybe_unused]] size_t  _history_size = 0;

        static UString _default_prompt;
        static UString _default_next_prompt;
        static UString _default_history_file;
        static size_t  _default_history_size;
    };
}
