//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsCommandLine.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::CommandLine::CommandLine(Report& report) :
    _report(report),
    _shell(),
    _process_redirections(false),
    _cmd_id_alloc(0),
    _cmd_enum(),
    _commands(),
    _predefined(*this)
{
}


//----------------------------------------------------------------------------
// Set command line redirection from files.
//----------------------------------------------------------------------------

bool ts::CommandLine::processRedirections(bool on)
{
    const bool previous = _process_redirections;
    _process_redirections = on;
    return previous;
}


//----------------------------------------------------------------------------
// Set the "shell" string for all commands.
//----------------------------------------------------------------------------

void ts::CommandLine::setShell(const UString& shell)
{
    _shell = shell;
    for (auto& it : _commands) {
        it.second.args.setShell(_shell);
    }
}


//----------------------------------------------------------------------------
// Set a new command line handler for one or all commands.
//----------------------------------------------------------------------------

void ts::CommandLine::setCommandLineHandlerImpl(CommandLineHandler* handler, CommandLineMethod method, const UString& name)
{
    if (name.empty()) {
        // Set all commands.
        for (auto& it : _commands) {
            it.second.handler = handler;
            it.second.method = method;
        }
    }
    else {
        // Set one command.
        const int id = _cmd_enum.value(name);
        if (id != Enumeration::UNKNOWN) {
            _commands[id].handler = handler;
            _commands[id].method = method;
        }
    }
}


//----------------------------------------------------------------------------
// Add the definition of a command to the interpreter.
//----------------------------------------------------------------------------

ts::Args* ts::CommandLine::commandImpl(CommandLineHandler* handler, CommandLineMethod method, const UString& name, const UString& description, const UString& syntax, int flags)
{
    // Check if the command already exists.
    int id = _cmd_enum.value(name, true, false);
    if (id == Enumeration::UNKNOWN) {
        // New command.
        id = _cmd_id_alloc++;
        _cmd_enum.add(name, id);
    }

    // Set the argument definition for the command.
    Cmd& cmd(_commands[id]);
    cmd.handler = handler;
    cmd.method = method;
    cmd.name = name;
    cmd.args.setDescription(description);
    cmd.args.setSyntax(syntax);
    cmd.args.setAppName(name);
    cmd.args.setShell(_shell);
    cmd.args.redirectReport(&_report);

    // Enforce flags to avoid exiting the application on special events (error or help).
    cmd.args.setFlags(flags |
                      Args::NO_EXIT_ON_HELP |
                      Args::NO_EXIT_ON_ERROR |
                      Args::HELP_ON_THIS |
                      Args::NO_DEBUG |
                      Args::NO_VERSION |
                      Args::NO_CONFIG_FILE);

    return &cmd.args;
}


//----------------------------------------------------------------------------
// Analyze a command line.
//----------------------------------------------------------------------------

bool ts::CommandLine::analyzeCommand(const UString& line)
{
    UStringVector args;
    line.fromQuotedLine(args);
    if (args.empty()) {
        return true; // empty command line
    }
    else {
        const UString cmd(args.front());
        args.erase(args.begin());
        return analyzeCommand(cmd, args);
    }
}

bool ts::CommandLine::analyzeCommand(const UString& name, const UStringVector& arguments)
{
    // Look for command name.
    const int cmd_id = _cmd_enum.value(name);
    if (cmd_id == Enumeration::UNKNOWN) {
        _report.error(_cmd_enum.error(name, true, true, u"command"));
        return false;
    }

    // Analyze command.
    return _commands[cmd_id].args.analyze(name, arguments, _process_redirections);
}


//----------------------------------------------------------------------------
// Analyze and process a command line.
//----------------------------------------------------------------------------

ts::CommandStatus ts::CommandLine::processCommand(const UString& line, Report* redirect)
{
    UStringVector args;
    line.fromQuotedLine(args);
    if (args.empty()) {
        return CommandStatus::SUCCESS; // empty command line
    }
    else {
        const UString cmd(args.front());
        args.erase(args.begin());
        return processCommand(cmd, args, redirect);
    }
}

ts::CommandStatus ts::CommandLine::processCommand(const UString& name, const UStringVector& arguments, Report* redirect)
{
    // Which log to use.
    Report* log = redirect != nullptr ? redirect : &_report;

    // Look for command name.
    const int cmd_id = _cmd_enum.value(name);
    if (cmd_id == Enumeration::UNKNOWN) {
        log->error(_cmd_enum.error(name, true, true, u"command"));
        return CommandStatus::ERROR;
    }

    // Analyze and process command.
    CommandStatus status = CommandStatus::SUCCESS;
    Cmd& cmd(_commands[cmd_id]);
    cmd.args.redirectReport(log);

    if (!cmd.args.analyze(cmd.name, arguments, _process_redirections)) {
        status = CommandStatus::ERROR;
    }
    else if (cmd.handler == nullptr || cmd.method == nullptr) {
        log->error(u"no command handler for command %s", {cmd.name});
        status = CommandStatus::ERROR;
    }
    else {
        status = (cmd.handler->*cmd.method)(cmd.name, cmd.args);
    }

    cmd.args.redirectReport(&_report);
    return status;
}


//----------------------------------------------------------------------------
// Check if we should continue executing commands.
//----------------------------------------------------------------------------

bool ts::CommandLine::more(CommandStatus status, bool exit_on_error) const
{
    return status != CommandStatus::EXIT && status != CommandStatus::FATAL && (!exit_on_error || status == CommandStatus::SUCCESS);
}


//----------------------------------------------------------------------------
// Analyze and process all commands from a text file.
//----------------------------------------------------------------------------

ts::CommandStatus ts::CommandLine::processCommandFiles(const UStringVector& file_names, bool exit_on_error, Report* redirect)
{
    CommandStatus status = CommandStatus::SUCCESS;
    for (size_t i = 0; more(status, exit_on_error) && i < file_names.size(); ++i) {
        status = processCommandFile(file_names[i]);
    }
    return status;
}

ts::CommandStatus ts::CommandLine::processCommandFile(const UString& file_name, bool exit_on_error, Report* redirect)
{
    _report.debug(u"executing commands from %s", {file_name});

    if (file_name.empty() || file_name == u"-") {
        // Execute an interactive session.
        return processInteractive(exit_on_error, redirect);
    }
    else {
        // Load all text lines from the file.
        ts::UStringVector lines;
        if (!UString::Load(lines, file_name)) {
            (redirect == nullptr ? _report : *redirect).error(u"error loading %s", {file_name});
            return CommandStatus::ERROR;
        }
        return processCommands(lines, exit_on_error, redirect);
    }
}


//----------------------------------------------------------------------------
// Analyze and process all commands from a vector of text lines.
//----------------------------------------------------------------------------

ts::CommandStatus ts::CommandLine::processCommands(UStringVector& lines, bool exit_on_error, Report* redirect)
{
    // Reduce comment and continuation lines.
    for (size_t i = 0; i < lines.size(); ) {
        lines[i].trim();
        if (lines[i].empty() || lines[i].startWith(u"#")) {
            // Comment line, drop it.
            lines.erase(lines.begin() + i);
        }
        else if (i > 0 && lines[i-1].endWith(u"\\")) {
            // Append as continuation of previous line and remove this line.
            lines[i-1].pop_back();
            lines[i-1].append(lines[i]);
            lines.erase(lines.begin() + i);
        }
        else {
            ++i;
        }
    }
    if (!lines.empty() && lines.back().endWith(u"\\")) {
        // Last line ends with backslash, error but ignore it, drop it.
        lines.back().pop_back();
        lines.back().trim();
    }

    // Execute all commands in sequence.
    CommandStatus status = CommandStatus::SUCCESS;
    for (size_t i = 0; more(status, exit_on_error) && i < lines.size(); ++i) {
        status = processCommand(lines[i], redirect);
    }
    return status;
}


//----------------------------------------------------------------------------
// Analyze and process all commands from an interactive session.
//----------------------------------------------------------------------------

ts::CommandStatus ts::CommandLine::processInteractive(bool exit_on_error, Report* redirect)
{
    return processInteractive(EditLine::DefaultPrompt(), EditLine::DefaultNextPrompt(), EditLine::DefaultHistoryFile(), EditLine::DefaultHistorySize(), exit_on_error, redirect);
}

ts::CommandStatus ts::CommandLine::processInteractive(const UString& prompt, const UString& next_prompt, const UString& history_file, size_t history_size, bool exit_on_error, Report* redirect)
{
    EditLine edit(prompt, next_prompt, history_file, history_size);
    UString line;
    CommandStatus status = CommandStatus::SUCCESS;
    while (more(status, exit_on_error) && edit.readLine(line)) {
        status = processCommand(line, redirect);
    }
    return status;
}

//----------------------------------------------------------------------------
// Build a list of command line definitions, sorted by name.
//----------------------------------------------------------------------------

void ts::CommandLine::getSortedCmd(std::vector<const Cmd*>& cmds) const
{
    cmds.clear();
    cmds.reserve(_commands.size());

    // Build a sorted list of command names.
    UStringVector names;
    _cmd_enum.getAllNames(names);
    std::sort(names.begin(), names.end());

    for (const auto& it1 : names) {
        const auto it2 = _commands.find(_cmd_enum.value(it1));
        if (it2 != _commands.end()) {
            cmds.push_back(&it2->second);
        }
    }
}


//----------------------------------------------------------------------------
// Get a formatted help text for all commands.
//----------------------------------------------------------------------------

ts::UString ts::CommandLine::getAllHelpText(Args::HelpFormat format, size_t line_width) const
{
    // Get sorted list of commands.
    std::vector<const Cmd*> cmds;
    getSortedCmd(cmds);

    UString text;
    for (size_t i = 0; i < cmds.size(); ++i) {
        // Get help for this command.
        UString help(cmds[i]->args.getHelpText(format, line_width));
        // Add a marker before the first non-space character to emphasize the start of command description.
        for (size_t i2 = 0; i2 < help.size(); ++i2) {
            if (!IsSpace(help[i2])) {
                help.insert(i2, u"==== ");
                break;
            }
        }
        text.append(help);
    }
    return text;
}


//----------------------------------------------------------------------------
// Add the predefined commands.
//----------------------------------------------------------------------------

void ts::CommandLine::addPredefinedCommands()
{
    command(&_predefined, &PredefinedCommands::help, u"help", u"List all internal commands", u"", Args::NO_VERBOSE);
    command(&_predefined, &PredefinedCommands::quit, u"exit", u"Exit command session", u"", Args::NO_VERBOSE);
    command(&_predefined, &PredefinedCommands::quit, u"quit", u"Exit command session", u"", Args::NO_VERBOSE);
}


//----------------------------------------------------------------------------
// Internal command handler for predefined commands.
//----------------------------------------------------------------------------

ts::CommandLine::PredefinedCommands::PredefinedCommands(CommandLine& cmdline) :
    _cmdline(cmdline)
{
}

ts::CommandLine::PredefinedCommands::~PredefinedCommands()
{
}

ts::CommandStatus ts::CommandLine::PredefinedCommands::quit(const UString& command, Args& args)
{
    return CommandStatus::EXIT;
}

ts::CommandStatus ts::CommandLine::PredefinedCommands::help(const UString& command, Args& args)
{
    // Get sorted list of commands.
    std::vector<const Cmd*> cmds;
    _cmdline.getSortedCmd(cmds);

    // Get max command name length.
    size_t width = 0;
    for (size_t i = 0; i < cmds.size(); ++i) {
        width = std::max(width, cmds[i]->name.width());
    }

    std::cout << std::endl;
    std::cout << "List of available commands:" << std::endl;
    std::cout << std::endl;

    for (size_t i = 0; i < cmds.size(); ++i) {
        std::cout << "  " << cmds[i]->name.toJustifiedLeft(width) << " : " << cmds[i]->args.getDescription() << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Use option --help on each command for more details" << std::endl;
    std::cout << std::endl;

    return CommandStatus::SUCCESS;
}
