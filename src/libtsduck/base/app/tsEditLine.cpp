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

#include "tsEditLine.h"
#include "tsFileUtils.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;

// Disable libedit on Windows.
#if defined(TS_WINDOWS) && !defined(TS_NO_EDITLINE)
#define TS_NO_EDITLINE 1
#endif

#if !defined(TS_NO_EDITLINE)
#include <editline/readline.h>
#endif


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::EditLine::EditLine(const UString& prompt, const UString& history_file, size_t history_size) :
    _isatty(StdInIsTerminal()),
    _eof(false),
    _prompt(prompt),
    _prompt_next(u"> "),
    _previous_line(),
    _history_file(history_file),
    _history_size(history_size),
    _line_count(0)
{
#if !defined(TS_NO_EDITLINE)
    if (_isatty) {
        ::using_history();
        if (_history_size > 0) {
            ::stifle_history(int(_history_size));
        }
        if (!_history_file.empty()) {
            ::read_history(_history_file.toUTF8().c_str());
        }
    }
#endif
}

ts::EditLine::~EditLine()
{
#if !defined(TS_NO_EDITLINE)
    if (_isatty && _line_count > 0 && !_history_file.empty()) {
        if (_history_size > 0) {
            ::stifle_history(int(_history_size));
        }
        ::write_history(_history_file.toUTF8().c_str());
    }
#endif
}


//----------------------------------------------------------------------------
// Get the default history file name.
//----------------------------------------------------------------------------

ts::UString ts::EditLine::DefaultHistoryFile()
{
#if defined(TS_WINDOWS)
    return GetEnvironment(u"APPDATA") + u"\\tsduck\\.tshistory";
#else
    return UserHomeDirectory() + u"/.tshistory";
#endif
}


//----------------------------------------------------------------------------
// Set a new number of history lines.
//----------------------------------------------------------------------------

void ts::EditLine::setHistoryFile(size_t history_size)
{
    _history_size = history_size;
#if !defined(TS_NO_EDITLINE)
    if (_isatty) {
        if (history_size > 0) {
            ::stifle_history(int(history_size));
        }
        else {
            ::unstifle_history();
        }
    }
#endif
}


//----------------------------------------------------------------------------
// Read one line of input.
//----------------------------------------------------------------------------

bool ts::EditLine::readLine(UString& line, bool skip_empty, bool trim, bool continuing)
{
    line.clear();

    bool read_more = true;
    UString* prompt = &_prompt;

    // Read multiple lines if continuing is true or skip empty lines.
    while (read_more && !_eof) {

        // Read one line of text.
        UString subline;
        if (!_isatty) {
            // Not a terminal, read from a pipe or file, no interaction.
            _eof = !subline.getLine(std::cin);
        }
        else {
#if defined(TS_NO_EDITLINE)
            // No libedit support, basic prompt + input.
            std::cout << (*prompt) << std::flush;
            _eof = !subline.getLine(std::cin);
#else
            // Read line with history and edition features.
            char* in = ::readline(prompt->toUTF8().c_str());
            _eof = in == nullptr;
            if (in != nullptr) {
                subline.assignFromUTF8(in);
                ::free(in);
            }
#endif
        }

        // Process that piece of line.
        if (trim) {
            subline.trim();
        }
        line.append(subline);
        if (continuing && line.endWith(u"\\")) {
            // Need to read a continuation line.
            line.pop_back();
            prompt = &_prompt_next;
        }
        else {
            // Line is complete, continue only if empty lines shall be ignored.
            read_more = skip_empty && line.empty();
        }
    }

#if !defined(TS_NO_EDITLINE)
    // Store full line in history, avoid successive duplicates.
    if (_isatty && !line.empty() && line != _previous_line) {
        ::add_history(line.toUTF8().c_str());
        _previous_line = line;
    }
#endif

    return !_eof || line.empty();
}
