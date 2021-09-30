//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
    //! @ingroup app
    //!
    class TSDUCKDLL EditLine
    {
    public:
        //!
        //! Get the default history file name.
        //! @return The default history file name.
        //!
        static UString DefaultHistoryFile();

        //!
        //! Constructor.
        //! @param [in] prompt Command line prompt.
        //! @param [in] history_file File to load/save the history.
        //! The history is loaded in the constructor and saved in the destructor.
        //! If empty, no history is loaded.
        //! @param [in] history_size Maximum number of history lines to save.
        //!
        EditLine(const UString& prompt = UString(), const UString& history_file = DefaultHistoryFile(), size_t history_size = 100);

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

        //!
        //! Set a new command line prompt.
        //! @param [in] prompt Command line prompt.
        //!
        void setPrompt(const UString& prompt) { _prompt = prompt; }

        //!
        //! Set a new command line prompt for continuation lines (after a backslash).
        //! @param [in] prompt Command line prompt for continuation lines.
        //! The initial default is "> ".
        //!
        void setNextPrompt(const UString& prompt) { _prompt_next = prompt; }

        //!
        //! Set a new history file.
        //! Used when the history shall be saved in a different file.
        //! @param [in] history_file File to save the history.
        //!
        void setHistoryFile(const UString& history_file) { _history_file = history_file; }

        //!
        //! Set a new number of history lines.
        //! @param [in] history_size Maximum number of history lines to save.
        //!
        void setHistoryFile(size_t history_size);

    private:
        bool    _isatty;
        bool    _eof;
        UString _prompt;
        UString _prompt_next;
        UString _previous_line;
        UString _history_file;
        size_t  _history_size;
        size_t  _line_count;
    };
}
