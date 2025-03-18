//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#include "tsNames.h"
#include "tsEditLine.h"
#include "tsCerrReport.h"

namespace ts {
    //!
    //! A basic command line interpreter.
    //! @ingroup libtscore cmd
    //!
    class TSCOREDLL CommandLine
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
        //! Destructor.
        //!
        virtual ~CommandLine();

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
        template <class HANDLER> requires std::derived_from<HANDLER, CommandLineHandler>
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
        //! @param [in] name Command name. If empty (the default), @a handler is set on all commands.
        //!
        template <class HANDLER> requires std::derived_from<HANDLER, CommandLineHandler>
        void setCommandLineHandler(HANDLER* handler, CommandStatus (HANDLER::*method)(const UString&, Args&), const UString& name = UString())
        {
            return setCommandLineHandlerImpl(handler, static_cast<CommandLineMethod>(method), name);
        }

        //!
        //! Add the predefined commands @e help, @e quit and @e exit.
        //! Useful for interactive interpreters.
        //!
        void addPredefinedCommands();

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
        //! @param [in] line Full command line, with command name and parameters.
        //! Parameters are separated with spaces. Special characters and spaces must be
        //! escaped or quoted in the parameters.
        //! @param [in] redirect If not null, temporarily redirect errors here.
        //! @return Command status.
        //!
        CommandStatus processCommand(const UString& line, Report* redirect = nullptr);

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
        //! @param [in] exit_on_error If true, exit command session on first error.
        //! @param [in] redirect If not null, temporarily redirect errors here.
        //! @return Command status of the last command.
        //!
        CommandStatus processCommands(UStringVector& lines, bool exit_on_error = false, Report* redirect = nullptr);

        //!
        //! Analyze and process all commands from a text file.
        //! Lines starting with a '#' are considered as comments and ignored.
        //! A line ending with a backslash means that the command continues on the next line.
        //! @param [in] file_name File name. If empty or "-", execute an interactive session from the standard input.
        //! @param [in] exit_on_error If true, exit command session on first error.
        //! @param [in] redirect If not null, temporarily redirect errors here.
        //! @return Command status of the last command.
        //!
        CommandStatus processCommandFile(const UString& file_name, bool exit_on_error = false, Report* redirect = nullptr);

        //!
        //! Analyze and process all commands from several text files.
        //! Lines starting with a '#' are considered as comments and ignored.
        //! A line ending with a backslash means that the command continues on the next line.
        //! @param [in] file_names Vector of file name. If one file name is empty or "-", execute an interactive session from the standard input.
        //! @param [in] exit_on_error If true, exit command session on first error.
        //! @param [in] redirect If not null, temporarily redirect errors here.
        //! @return Command status of the last command.
        //!
        CommandStatus processCommandFiles(const UStringVector& file_names, bool exit_on_error = false, Report* redirect = nullptr);

        //!
        //! Analyze and process all commands from an interactive session.
        //! @param [in] prompt Command line prompt.
        //! @param [in] next_prompt Command line prompt for continuation lines.
        //! @param [in] history_file File to load/save the history. If empty, no history is loaded or saved.
        //! @param [in] history_size Maximum number of history lines to save.
        //! @param [in] exit_on_error If true, exit command session on first error.
        //! @param [in] redirect If not null, temporarily redirect errors here.
        //! @return Command status of the last command.
        //! @see EditLine
        //!
        CommandStatus processInteractive(const UString& prompt = EditLine::DefaultPrompt(),
                                         const UString& next_prompt = EditLine::DefaultNextPrompt(),
                                         const UString& history_file = EditLine::DefaultHistoryFile(),
                                         size_t history_size = EditLine::DefaultHistorySize(),
                                         bool exit_on_error = false,
                                         Report* redirect = nullptr);

        //!
        //! Analyze and process all commands from an interactive session.
        //! Use all defaults for prompts and history.
        //! @param [in] exit_on_error If true, exit command session on first error.
        //! @param [in] redirect If not null, temporarily redirect errors here.
        //! @return Command status of the last command.
        //! @see EditLine
        //!
        CommandStatus processInteractive(bool exit_on_error, Report* redirect = nullptr);

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
            CommandLineHandler* handler = nullptr;
            CommandLineMethod method = nullptr;
            UString name {};
            Args    args {};
        public:
            Cmd() = default;
        };

        // Internal command handler for predefined commands.
        class PredefinedCommands : public CommandLineHandler
        {
            TS_NOBUILD_NOCOPY(PredefinedCommands);
        public:
            PredefinedCommands(CommandLine& cmdline);
            virtual ~PredefinedCommands() override;
            CommandStatus help(const UString&, Args&);
            CommandStatus quit(const UString&, Args&);
        private:
            CommandLine& _cmdline;
        };

        // Command id.
        using CmdId = Names::int_t;

        // CommandLine private members.
        Report&             _report;
        UString             _shell {};
        bool                _process_redirections = false;
        CmdId               _cmd_id_alloc = 0;     // sequential allocator of command ids.
        Names               _cmd_enum {};          // commands name and ids, used to handle abbreviated command names.
        PredefinedCommands  _predefined;           // predefined commands handler.
        std::map<CmdId,Cmd> _commands {};          // command ids to arguments.

        // Build a list of command line definitions, sorted by name.
        void getSortedCmd(std::vector<const Cmd*>&) const;

        // Check if we should continue executing commands.
        bool more(CommandStatus status, bool exit_on_error) const;

        // Non-template implementations of command registrations.
        Args* commandImpl(CommandLineHandler* handler, CommandLineMethod method, const UString& name, const UString& description, const UString& syntax, int flags);
        void setCommandLineHandlerImpl(CommandLineHandler* handler, CommandLineMethod method, const UString& command);
    };
}
