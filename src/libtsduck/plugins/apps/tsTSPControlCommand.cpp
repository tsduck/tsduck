//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTSPControlCommand.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TSPControlCommand::TSPControlCommand(Report& report) :
    CommandLine(report)
{
    // Define the syntax for all commands.
    Args* arg = nullptr;
    const int flags = Args::NO_HELP;

    arg = command(u"exit", u"Terminate the tsp process", u"[options]", flags | Args::NO_VERBOSE);
    arg->option(u"abort");
    arg->help(u"abort",
              u"Specify to immediately abort the tsp process. "
              u"By default, this command notifies each plugin to terminate "
              u"and let the processing continue until the process naturally exits.");

    arg = command(u"set-log", u"Change log level in the tsp process", u"level", flags | Args::NO_VERBOSE);
    arg->option(u"", 0, Severity::Enums, 1, 1);
    arg->help(u"",
              u"Specify a new logging level for the tsp process. "
              u"It can be either a name or a positive value for higher debug levels.");

    arg = command(u"list", u"List all running plugins", u"[options]", flags);

    arg = command(u"suspend", u"Suspend a plugin", u"[options] plugin-index", flags);
    arg->setIntro(u"Suspend a plugin. When a packet processing plugin is suspended, "
                  u"the TS packets are directly passed from the previous to the next plugin, "
                  u"without going through the suspended one. When the output plugin is suspended, "
                  u"the output packets are dropped. The input plugin cannot be suspended. "
                  u"Use the command 'list' to list all running plugins. ");
    arg->option(u"", 0, Args::UNSIGNED);
    arg->help(u"", u"Index of the plugin to suspend.");

    arg = command(u"resume", u"Resume a suspended plugin", u"[options] plugin-index", flags);
    arg->option(u"", 0, Args::UNSIGNED);
    arg->help(u"", u"Index of the plugin to resume.");

    arg = command(u"restart", u"Restart plugin with different parameters", u"[options] plugin-index [plugin-options ...]", flags | Args::GATHER_PARAMETERS);
    arg->option(u"", 0, Args::STRING, 1, Args::UNLIMITED_COUNT);
    arg->help(u"",
              u"Index of the plugin to restart, followed by the new plugin parameters to use.");
    arg->option(u"same", 's');
    arg->help(u"same",
              u"Restart the plugin with the same options and parameters. "
              u"By default, when no plugin options are specified, restart with no option at all.");
}
