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
#include "tsCommandLineHandler.h"
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
        //! Set the "shell" string for all commands.
        //! @param [in] shell Shell name string.
        //! @see Args::setShell()
        //!
        void setShell(const UString& shell);

        //!
        //! Add the definition of a command to the interpreter, without command handler.
        //! @param [in] name Command name.
        //! @param [in] description A short one-line description of the command.
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //! @param [in] flags An or'ed mask of Args::Flags values.
        //! @return A pointer to the Args for this command. The application will typically add options to this Args.
        //! @see Args::Flags
        //! @see Args::Args()
        //!
        Args* command(const UString& name, const UString& description = UString(), const UString& syntax = UString(), int flags = 0)
        {
            return commandImpl(nullptr, nullptr, name, description, syntax, flags);
        }

        //!
        //! Add the definition of a command to the interpreter.
        //! @tparam HANDLER A subclass of CommandLineHandler.
        //! @param [in] handler Command handler object. A null pointer is allowed (unhandled command).
        //! @param [in] method Command handler method. A null pointer is allowed (unhandled command).
        //! @param [in] name Command name.
        //! @param [in] description A short one-line description of the command.
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //! @param [in] flags An or'ed mask of Args::Flags values.
        //! @return A pointer to the Args for this command. The application will typically add options to this Args.
        //! @see Args::Flags
        //! @see Args::Args()
        //!
        template <class HANDLER, typename std::enable_if<std::is_base_of<CommandLineHandler, HANDLER>::value, int>::type = 0>
        Args* command(HANDLER* handler,
                      CommandStatus (HANDLER::*method)(const UString&, Args&),
                      const UString& name,
                      const UString& description = UString(),
                      const UString& syntax = UString(),
                      int flags = 0)
        {
            return commandImpl(handler, static_cast<CommandLineMethod>(method), name, description, syntax, flags);
        }

        //!
        //! Set a new command line handler for one or all commands.
        //! @tparam HANDLER A subclass of CommandLineHandler.
        //! @param [in] handler Command handler object. A null pointer is allowed (unhandled command).
        //! @param [in] method Command handler method. A null pointer is allowed (unhandled command).
        //! @param [in] command Command name. If empty (the default), @a handler is set on all commands.
        //!
        template <class HANDLER, typename std::enable_if<std::is_base_of<CommandLineHandler, HANDLER>::value, int>::type = 0>
        void setCommandLineHandler(HANDLER* handler, CommandStatus (HANDLER::*method)(const UString&, Args&), const UString& command = UString())
        {
            return setCommandLineHandlerImpl(handler, static_cast<CommandLineMethod>(method), command);
        }

        //!
        //! Set command line redirection from files.
        //! @param [in] on If true, process command line arguments redirection.
        //! All lines with the form @c '\@filename' are replaced by the content
        //! The initial default is false.
        //! @return The previous state of the redirections.
        //!
        bool processRedirections(bool on);

        //!
        //! Analyze a command line.
        //! @param [in] command Full command line, with command name and parameters.
        //! Parameters are separated with spaces. Special characters and spaces must be
        //! escaped or quoted in the parameters.
        //! @return True if the command is correct, false otherwise.
        //!
        bool analyzeCommand(const UString& command);

        //!
        //! Analyze a command line.
        //! @param [in] name Command name.
        //! @param [in] arguments Arguments from command line.
        //! @return True if the command is correct, false otherwise.
        //!
        bool analyzeCommand(const UString& name, const UStringVector& arguments);

        //!
        //! Analyze and process a command line.
        //! @param [in] command Full command line, with command name and parameters.
        //! Parameters are separated with spaces. Special characters and spaces must be
        //! escaped or quoted in the parameters.
        //! @param [in] redirect If not null, temporarily redirect errors here.
        //! @return Command status.
        //!
        CommandStatus processCommand(const UString& command, Report* redirect = nullptr);

        //!
        //! Analyze and process a command line.
        //! @param [in] name Command name.
        //! @param [in] arguments Arguments from command line.
        //! @param [in] redirect If not null, temporarily redirect errors here.
        //! @return Command status.
        //!
        CommandStatus processCommand(const UString& name, const UStringVector& arguments, Report* redirect = nullptr);

        //!
        //! Analyze and process all commands from a vector of text lines.
        //! @param [in,out] lines Lines of text. Lines starting with a '#' are considered as
        //! comments and removed. A line ending with a backslash means that the command
        //! continues on the next line. Such lines are reassembled.
        //! @param [in] redirect If not null, temporarily redirect errors here.
        //! @return Command status of the last command.
        //!
        CommandStatus processCommandFile(UStringVector& lines, Report* redirect = nullptr);

        //!
        //! Analyze and process all commands from a text file.
        //! Lines starting with a '#' are considered as comments and ignored.
        //! A line ending with a backslash means that the command continues on the next line.
        //! @param [in] filename File name.
        //! @param [in] redirect If not null, temporarily redirect errors here.
        //! @return Command status of the last command.
        //!
        CommandStatus processCommandFile(const UString& filename, Report* redirect = nullptr);

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
            CommandLineHandler* handler;
            CommandLineMethod method;
            UString name;
            Args    args;
            Cmd() : handler(nullptr), method(nullptr), name(), args() {}
        };

        // CommandLine private members.
        Report&           _report;
        UString           _shell;
        bool              _process_redirections;
        int               _cmd_id_alloc;  // sequential allocator of command ids.
        Enumeration       _cmd_enum;      // commands name and ids, used to handle abbreviated command names.
        std::map<int,Cmd> _commands;      // command ids to arguments.

        // Non-template implementations of command registrations.
        Args* commandImpl(CommandLineHandler* handler, CommandLineMethod method, const UString& name, const UString& description, const UString& syntax, int flags);
        void setCommandLineHandlerImpl(CommandLineHandler* handler, CommandLineMethod method, const UString& command);
    };
}
