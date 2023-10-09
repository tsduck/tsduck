//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEditLine.h"
#include "tsFileUtils.h"
#include "tsSysUtils.h"

// Disable libedit on Windows.
#if defined(TS_WINDOWS) && !defined(TS_NO_EDITLINE)
    #define TS_NO_EDITLINE 1
#endif

#if !defined(TS_NO_EDITLINE)
    #include "tsBeforeStandardHeaders.h"
    #if defined(TS_OPENBSD)
        #include <readline/readline.h>
        #include <readline/history.h>
    #else
        #include <editline/readline.h>
    #endif
    #include "tsAfterStandardHeaders.h"
#endif

ts::UString ts::EditLine::_default_prompt(u"> ");
ts::UString ts::EditLine::_default_next_prompt(u">>> ");
ts::UString ts::EditLine::_default_history_file(ts::UserConfigurationFileName(u".tshistory"));
size_t      ts::EditLine::_default_history_size(100);


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::EditLine::EditLine(const UString& prompt, const UString& next_prompt, const UString& history_file, size_t history_size) :
    _is_a_tty(StdInIsTerminal()),
    _prompt(prompt),
    _next_prompt(next_prompt),
    _history_file(history_file),
    _history_size(history_size)
{
#if !defined(TS_NO_EDITLINE)
    if (_is_a_tty) {
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
    if (_is_a_tty && _update_history && !_history_file.empty()) {
        if (_history_size > 0) {
            ::stifle_history(int(_history_size));
        }
        ::write_history(_history_file.toUTF8().c_str());
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
    while (read_more && !_end_of_file) {

        // Read one line of text.
        UString subline;
        if (!_is_a_tty) {
            // Not a terminal, read from a pipe or file, no interaction.
            _end_of_file = !subline.getLine(std::cin);
        }
        else {
#if defined(TS_NO_EDITLINE)
            // No libedit support, basic prompt + input.
            std::cout << (*prompt) << std::flush;
            _end_of_file = !subline.getLine(std::cin);
#else
            // Read line with history and edition features.
            char* in = ::readline(prompt->toUTF8().c_str());
            _end_of_file = in == nullptr;
            if (in == nullptr) {
                // With libeditline, enforce a new-line on eof.
                std::cout << std::endl;
            }
            else {
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
            prompt = &_next_prompt;
        }
        else {
            // Line is complete, continue only if empty lines shall be ignored.
            read_more = skip_empty && line.empty();
        }
    }

#if !defined(TS_NO_EDITLINE)
    // Store full line in history, avoid successive duplicates.
    if (_is_a_tty && !line.empty() && line != _previous_line) {
        ::add_history(line.toUTF8().c_str());
        _previous_line = line;
        _update_history = true;
    }
#endif

    return !_end_of_file || !line.empty();
}
