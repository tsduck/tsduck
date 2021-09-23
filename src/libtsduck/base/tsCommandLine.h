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
//!  Command line interpreter.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsCommandLineHandlerInterface.h"
#include "tsEnumeration.h"
#include "tsCerrReport.h"

namespace ts {
    //!
    //! A basic command line interpreter.
    //! @ingroup cmd
    //!
    class TSDUCKDLL CommandLine
    {
        TS_NOCOPY(CommandLine);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Reference to a report where all messages are displayed.
        //! The reference must remain valid as long as this object exists.
        //!
        CommandLine(Report& report = CERR);

        //!
        //! Add the definition of a command to the interpreter.
        //! @param [in] handler Command handler.
        //! @param [in] name Command name.
        //! @param [in] description A short one-line description of the command.
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //! @param [in] flags An or'ed mask of Args::Flags values.
        //! @return A pointer to the Args for this command. The application will typically add options to this Args.
        //! @see Args::Flags
        //! @see Args::Args()
        //!
        Args* command(CommandLineHandlerInterface* handler,
                      const UString& name,
                      const UString& description = UString(),
                      const UString& syntax = UString(),
                      int flags = 0);

        //!
        //! Set a new command line handler for one or all commands.
        //! @param [in] handler New handler to set.
        //! @param [in] command Command name. If empty (the default), @a handler is set on all commands.
        //!
        void setCommandLineHandler(CommandLineHandlerInterface* handler, const UString& command = UString());

        //!
        //! Set command line redirection from files.
        //! @param [in] on If true, process command line arguments redirection.
        //! All lines with the form @c '\@filename' are replaced by the content
        //! The initial default is false.
        //!
        bool processRedirections(bool on);

        //!
        //! Analyze and process a command line.
        //! @param [in] command Full command line, with command name and parameters.
        //! Parameters are separated with spaces. Special characters and spaces must be
        //! escaped or quoted in the parameters.
        //! @return True if the command was successfully processed. False if the command
        //! does not exist or the command handler returned false.
        //!
        bool processCommand(const UString& command);

        //!
        //! Analyze and process a command line.
        //! @param [in] name Command name.
        //! @param [in] arguments Arguments from command line.
        //! @return True if the command was successfully processed. False if the command
        //! does not exist or the command handler returned false.
        //!
        bool processCommand(const UString& name, const UStringVector& arguments);

        //!
        //! Get a formatted help text for all commands.
        //! @param [in] format Requested format of the help text.
        //! @param [in] line_width Maximum width of text lines.
        //! @return The formatted help text.
        //!
        UString getAllHelpText(Args::HelpFormat format, size_t line_width = 79) const;

    private:
        // Definition of a command.
        class Cmd
        {
            TS_NOCOPY(Cmd);
        public:
            CommandLineHandlerInterface* handler;
            UString name;
            Args    args;
            Cmd() : handler(nullptr), name(), args() {}
        };

        // CommandLine private members.
        Report&           _report;
        bool              _process_redirections;
        int               _cmd_id_alloc;  // sequential allocator of command ids.
        Enumeration       _cmd_enum;      // commands name and ids, used to handle abbreviated command names.
        std::map<int,Cmd> _commands;      // command ids to arguments.
    };
}

#include "tsCommandLineTemplate.h"
