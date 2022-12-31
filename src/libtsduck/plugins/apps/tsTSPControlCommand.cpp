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
