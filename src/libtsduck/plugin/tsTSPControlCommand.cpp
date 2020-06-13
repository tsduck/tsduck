//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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

#include "tsTSPControlCommand.h"
TSDUCK_SOURCE;

// Enumeration description of ControlCommand.
const ts::Enumeration ts::TSPControlCommand::ControlCommandEnum({
    {u"exit",    ts::TSPControlCommand::ControlCommand::CMD_EXIT},
    {u"set-log", ts::TSPControlCommand::ControlCommand::CMD_SETLOG},
    {u"list",    ts::TSPControlCommand::ControlCommand::CMD_LIST},
    {u"suspend", ts::TSPControlCommand::ControlCommand::CMD_SUSPEND},
    {u"resume",  ts::TSPControlCommand::ControlCommand::CMD_RESUME},
    {u"restart", ts::TSPControlCommand::ControlCommand::CMD_RESTART},
});


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TSPControlCommand::TSPControlCommand() :
    _commands()
{
    // Define the syntax for all commands.
    Args* arg = nullptr;

    arg = newCommand(CMD_EXIT, u"Terminate the tsp process", u"[options]", Args::NO_VERBOSE);
    arg->option(u"abort");
    arg->help(u"abort",
              u"Specify to immediately abort the tsp process. "
              u"By default, this command notifies each plugin to terminate "
              u"and let the processing continue until the process naturally exits.");

    arg = newCommand(CMD_SETLOG, u"Change log level in the tsp process", u"level", Args::NO_VERBOSE);
    arg->option(u"", 0, Severity::Enums, 1, 1);
    arg->help(u"",
              u"Specify a new logging level for the tsp process. "
              u"It can be either a name or a positive value for higher debug levels.");

    arg = newCommand(CMD_LIST, u"List all running plugins", u"[options]");

    arg = newCommand(CMD_SUSPEND, u"Suspend a plugin", u"[options] plugin-index");
    arg->setIntro(u"Suspend a plugin. When a packet processing plugin is suspended, "
                  u"the TS packets are directly passed from the previous to the next plugin, "
                  u"without going through the suspended one. When the output plugin is suspended, "
                  u"the output packets are dropped. The input plugin cannot be suspended. "
                  u"Use the command " + ControlCommandEnum.name(CMD_LIST) + u" to list all running plugins. ");
    arg->option(u"", 0, Args::UNSIGNED);
    arg->help(u"", u"Index of the plugin to suspend.");

    arg = newCommand(CMD_RESUME, u"Resume a suspended plugin", u"[options] plugin-index");
    arg->option(u"", 0, Args::UNSIGNED);
    arg->help(u"", u"Index of the plugin to resume.");

    arg = newCommand(CMD_RESTART, u"Restart plugin with different parameters", u"[options] plugin-index [plugin-options ...]", Args::GATHER_PARAMETERS);
    arg->option(u"", 0, Args::STRING, 1, Args::UNLIMITED_COUNT);
    arg->help(u"",
              u"Index of the plugin to restart, followed by the new plugin parameters to use.");
    arg->option(u"same", 's');
    arg->help(u"same",
              u"Restart the plugin with the same options and parameters. "
              u"By default, when no plugin options are specified, restart with no option at all.");
}


//----------------------------------------------------------------------------
// Add a new command.
//----------------------------------------------------------------------------

ts::Args* ts::TSPControlCommand::newCommand(ControlCommand cmd, const UString& description, const UString& syntax, int flags)
{
    Args* arg = &_commands[cmd];

    arg->setDescription(description);
    arg->setSyntax(syntax);
    arg->setShell(u"tspcontrol");
    arg->setAppName(ControlCommandEnum.name(cmd));

    arg->setFlags(flags |
                  Args::NO_EXIT_ON_HELP |
                  Args::NO_EXIT_ON_ERROR |
                  Args::NO_EXIT_ON_VERSION |
                  Args::HELP_ON_THIS |
                  Args::NO_DEBUG |
                  Args::NO_VERSION |
                  Args::NO_HELP |
                  Args::NO_CONFIG_FILE);

    return arg;
}


//----------------------------------------------------------------------------
// Analyze a command line.
//----------------------------------------------------------------------------

bool ts::TSPControlCommand::analyze(const UString& line, ControlCommand& cmd, const Args*& args, Report& report)
{
    // Split the command.
    UString name;
    UStringVector params;
    line.fromQuotedLine(params);

    if (params.empty()) {
        report.error(u"no control command specified");
        return false;
    }

    name = params.front();
    params.erase(params.begin());

    // Search the command.
    const int value = ControlCommandEnum.value(name, false);
    const auto it = _commands.find(ControlCommand(value));
    if (it == _commands.end()) {
        report.error(u"unknown control command: %s", {name});
        return false;
    }
    cmd = ControlCommand(value);
    args = &it->second;

    // Analyze the command.
    // Temporarily redirects the error reporting of the command analysis.
    it->second.redirectReport(&report);
    const bool ok = it->second.analyze(name, params, false);
    it->second.redirectReport(nullptr);
    return ok;
}


//----------------------------------------------------------------------------
// Get a formatted help text for all commands.
//----------------------------------------------------------------------------

ts::UString ts::TSPControlCommand::getAllHelpText(Args::HelpFormat format, size_t line_width) const
{
    // Build a sorted list of command names.
    UStringVector names;
    for (auto it = ControlCommandEnum.begin(); it != ControlCommandEnum.end(); ++it) {
        names.push_back(it->second);
    }
    std::sort(names.begin(), names.end());

    // Build a text of all helps.
    UString text;
    for (auto it1 = names.begin(); it1 != names.end(); ++it1) {
        const int cmd = ControlCommandEnum.value(*it1);
        if (cmd != Enumeration::UNKNOWN) {
            const auto it2 = _commands.find(ControlCommand(cmd));
            if (it2 != _commands.end()) {
                // Get help for this command.
                UString help(it2->second.getHelpText(format, line_width));
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
    }
    return text;
}
