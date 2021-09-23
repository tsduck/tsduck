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
//
//  EIT manipulation tool.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsCommandLine.h"
#include "tsEITGenerator.h"
TSDUCK_SOURCE;
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
// Command line options
//----------------------------------------------------------------------------

namespace {
    class Options: public ts::Args
    {
        TS_NOBUILD_NOCOPY(Options);
    public:
        Options(int argc, char *argv[]);

        ts::UString       direct_command;
        ts::UStringVector direct_args;
        ts::UStringVector command_files;
        ts::CommandLine   commands;

        // Inherited methods.
        virtual ts::UString getHelpText(HelpFormat format, size_t line_width = DEFAULT_LINE_WIDTH) const override;
    };
}

// Get command line options.
Options::Options(int argc, char *argv[]) :
    ts::Args(u"Manipulate EIT's through commands", u"[options] [command ...]", GATHER_PARAMETERS),
    direct_command(),
    direct_args(),
    command_files(),
    commands(*this)
 {
    // Command line options.
    option(u"", 0, STRING, 0, UNLIMITED_COUNT);
    help(u"", u"An optional EIT manipulation command.");

    option(u"execute", 'e', STRING, 0, UNLIMITED_COUNT);
    help(u"execute", u"filename",
         u"Specify a text file containing EIT manipulation commands to execute. "
         u"Several --execute options can be specified. All files are executed in sequence. "
         u"The command files are executed first, then the optional direct command is executed.");

    // EIT manipulation commands.
    ts::Args* cmd = nullptr;

    cmd = commands.command(nullptr, u"load", u"Load events from a file", u"[options] filename");
    cmd->option(u"", 0, STRING, 1, 1);
    cmd->help(u"", u"A binary, XML or JSON file containing EIT sections.");

    cmd = commands.command(nullptr, u"save", u"Save all current EIT sections in a file", u"[options] filename");
    cmd->option(u"", 0, STRING, 1, 1);
    cmd->help(u"", u"Name of the output file receiving EIT sections in binary format.");

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    getValues(command_files, u"execute");
    getValue(direct_command, u"");
    const size_t max = count(u"");
    for (size_t i = 1; i < max; ++i) {
        direct_args.push_back(value(u"", u"", i));
    }

    // At least one command shall be specified.
    if (direct_command.empty() && command_files.empty()) {
        error(u"no command specified");
    }

    // Final checking
    exitOnError();
}

// Build full help text.
ts::UString Options::getHelpText(HelpFormat format, size_t line_width) const
{
    // Initial text from superclass.
    ts::UString text(Args::getHelpText(format, line_width));

    // If full help, add help for all commands.
    if (format == HELP_FULL) {
        text.append(u"\nEIT manipulation commands:\n");
        const size_t margin = line_width > 10 ? 2 : 0;
        text.append(commands.getAllHelpText(HELP_FULL, line_width - margin).toIndented(margin));
    }
    return text;
}


//----------------------------------------------------------------------------
// A class to manipulate the EIT database.
//----------------------------------------------------------------------------

namespace {
    class Database : public ts::CommandLineHandlerInterface
    {
        TS_NOBUILD_NOCOPY(Database);
    public:
        // Constructor.
        Database(Options& opt);

        // Implementation of CommandLineHandlerInterface.
        virtual bool handleCommandLine(const ts::UString& command, ts::Args& args) override;

    private:
        Options&         _opt;
        ts::DuckContext  _duck;
        ts::EITGenerator _eitgen;
    };
}


//----------------------------------------------------------------------------
// Database constructor.
//----------------------------------------------------------------------------

Database::Database(Options& opt) :
    _opt(opt),
    _duck(&_opt),
    _eitgen(_duck)
{
    // Connect this object as command handler for all commands.
    _opt.commands.setCommandLineHandler(this);
}


//----------------------------------------------------------------------------
// Database command handler, implementation of CommandLineHandlerInterface.
//----------------------------------------------------------------------------

bool Database::handleCommandLine(const ts::UString& command, ts::Args& args)
{
    if (command == u"load") {
        //@@@
    }
    else if (command == u"save") {
        //@@@
    }
    return true;
}


//----------------------------------------------------------------------------
// Program main code.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Get command line options.
    Options opt(argc, argv);
    Database dbase(opt);

    // Execute all command files first.
    //@@@

    // Execute direct command if present.
    //@@@

    return EXIT_SUCCESS;
}
