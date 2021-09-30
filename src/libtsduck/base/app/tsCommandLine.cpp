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

#include "tsCommandLine.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::CommandLine::CommandLine(Report& report) :
    _report(report),
    _shell(),
    _process_redirections(false),
    _cmd_id_alloc(0),
    _cmd_enum(),
    _commands()
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
    for (auto it = _commands.begin(); it != _commands.end(); ++it) {
        it->second.args.setShell(_shell);
    }
}


//----------------------------------------------------------------------------
// Set a new command line handler for one or all commands.
//----------------------------------------------------------------------------

void ts::CommandLine::setCommandLineHandlerImpl(CommandLineHandler* handler, CommandLineMethod method, const UString& command)
{
    if (command.empty()) {
        // Set all commands.
        for (auto it = _commands.begin(); it != _commands.end(); ++it) {
            it->second.handler = handler;
            it->second.method = method;
        }
    }
    else {
        // Set one command.
        const int id = _cmd_enum.value(command);
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

bool ts::CommandLine::analyzeCommand(const UString& command)
{
    UStringVector args;
    command.fromQuotedLine(args);
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

ts::CommandStatus ts::CommandLine::processCommand(const UString& command, Report* redirect)
{
    UStringVector args;
    command.fromQuotedLine(args);
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
// Analyze and process all commands from a text file.
//----------------------------------------------------------------------------

ts::CommandStatus ts::CommandLine::processCommandFile(const UString& filename, Report* redirect)
{
    _report.debug(u"executing commands from %s", {filename});

    // Load all text lines from the file.
    ts::UStringVector lines;
    if (!UString::Load(lines, filename)) {
        (redirect == nullptr ? _report : *redirect).error(u"error loading %s", {filename});
        return CommandStatus::ERROR;
    }
    return processCommandFile(lines, redirect);
}

ts::CommandStatus ts::CommandLine::processCommandFile(UStringVector& lines, Report* redirect)
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
    for (size_t i = 0; status != CommandStatus::EXIT && status != CommandStatus::FATAL && i < lines.size(); ++i) {
        status = processCommand(lines[i], redirect);
    }
    return status;
}


//----------------------------------------------------------------------------
// Get a formatted help text for all commands.
//----------------------------------------------------------------------------

ts::UString ts::CommandLine::getAllHelpText(Args::HelpFormat format, size_t line_width) const
{
    // Build a sorted list of command names.
    UStringVector names;
    _cmd_enum.getAllNames(names);
    std::sort(names.begin(), names.end());

    // Build a text of all helps.
    UString text;
    for (auto it1 = names.begin(); it1 != names.end(); ++it1) {
        const auto it2 = _commands.find(_cmd_enum.value(*it1));
        if (it2 != _commands.end()) {
            // Get help for this command.
            UString help(it2->second.args.getHelpText(format, line_width));
            // Add a marker before the first non-space character to emphasize the start of command description.
            for (size_t i = 0; i < help.size(); ++i) {
                if (!IsSpace(help[i])) {
                    help.insert(i, u"==== ");
                    break;
                }
            }
            text.append(help);
        }
    }
    return text;
}
