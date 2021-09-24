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
#include "tsTSFile.h"
#include "tsFileUtils.h"
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
        ts::UString       input_directory;
        ts::UString       output_directory;
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
    input_directory(),
    output_directory(),
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

    option(u"input-directory", 'i', STRING);
    help(u"input-directory", u"path",
         u"Default directory of input files in EIT manipulation commands.");

    option(u"output-directory", 'i', STRING);
    help(u"output-directory", u"path",
         u"Default directory of output files in EIT manipulation commands.");

    // EIT manipulation commands.
    ts::Args* cmd = nullptr;
    const int flags = ts::Args::NO_VERBOSE | ts::Args::NO_HELP;

    cmd = commands.command(nullptr, u"load", u"Load events from a file", u"filename", flags);
    cmd->option(u"", 0, STRING, 1, 1);
    cmd->help(u"", u"A binary, XML or JSON file containing EIT sections.");

    cmd = commands.command(nullptr, u"save", u"Save all current EIT sections in a file", u"filename", flags);
    cmd->option(u"", 0, STRING, 1, 1);
    cmd->help(u"", u"Name of the output file receiving EIT sections in binary format.");

    cmd = commands.command(nullptr, u"generate", u"Generate TS packets", u"[options] filename", flags);
    cmd->option(u"", 0, STRING, 1, 1);
    cmd->help(u"", u"Name of the output TS file to generate.");
    cmd->option(u"bytes", 'b', POSITIVE);
    cmd->help(u"bytes", u"Size of the TS file in bytes.");
    cmd->option(u"packets", 'p', POSITIVE);
    cmd->help(u"packets", u"Number of TS packets to generate.");
    cmd->option(u"seconds", 's', POSITIVE);
    cmd->help(u"seconds", u"Duration in seconds of the file to generate.");

    commands.command(nullptr, u"reset", u"Reset the content of the EIT database", u"", flags);

    commands.command(nullptr, u"dump", u"Dump the content of the EIT database", u"", flags);

    cmd = commands.command(nullptr, u"set", u"Set EIT generation options", u"[options]", flags);
    cmd->option(u"terrestrial");
    cmd->help(u"terrestrial", u"Use the EIT cycle profile for terrestrial networks as specified in ETSI TS 101 211.");
    cmd->option(u"satellite");
    cmd->help(u"satellite", u"Use the EIT cycle profile for satellite and cable networks as specified in ETSI TS 101 211.");
    cmd->option(u"pf");
    cmd->help(u"pf", u"Enable the generation of EIT p/f.");
    cmd->option(u"no-pf");
    cmd->help(u"no-pf", u"Disable the generation of EIT p/f.");
    cmd->option(u"schedule");
    cmd->help(u"schedule", u"Enable the generation of EIT schedule.");
    cmd->option(u"no-schedule");
    cmd->help(u"no-schedule", u"Disable the generation of EIT schedule.");
    cmd->option(u"actual");
    cmd->help(u"actual", u"Enable the generation of EIT actual.");
    cmd->option(u"no-actual");
    cmd->help(u"no-actual", u"Disable the generation of EIT actual.");
    cmd->option(u"other");
    cmd->help(u"other", u"Enable the generation of EIT other.");
    cmd->option(u"no-other");
    cmd->help(u"no-other", u"Disable the generation of EIT other.");
    cmd->option(u"ts-id", 0, UINT16);
    cmd->help(u"ts-id", u"Set the actual transport stream id.");
    cmd->option<ts::BitRate>(u"ts-bitrate");
    cmd->help(u"ts-bitrate", u"Set the transport stream bitrate in bits/second.");
    cmd->option<ts::BitRate>(u"eit-bitrate");
    cmd->help(u"eit-bitrate", u"Set the EIT maximum bitrate in bits/second.");
    cmd->option(u"time", 0, STRING);
    cmd->help(u"time", u"year/month/day:hour:minute:second.millisecond", u"Set the current time");

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    getValues(command_files, u"execute");
    getValue(input_directory, u"input-directory");
    getValue(output_directory, u"output-directory");
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
        virtual ts::CommandStatus handleCommandLine(const ts::UString& command, ts::Args& args) override;

    private:
        Options&         _opt;
        ts::DuckContext  _duck;
        ts::BitRate      _ts_bitrate;
        ts::EITOption    _eit_options;
        ts::EITGenerator _eit_gen;

        // Get full path of an input or output directory.
        ts::UString fileName(const ts::UString& directory, const ts::UString& name) const;
        ts::UString inputFileName(const ts::UString& name) const { return fileName(_opt.input_directory, name); }
        ts::UString outputFileName(const ts::UString& name) const { return fileName(_opt.output_directory, name); }
    };
}


//----------------------------------------------------------------------------
// Database constructor.
//----------------------------------------------------------------------------

Database::Database(Options& opt) :
    _opt(opt),
    _duck(&_opt),
    _ts_bitrate(0),
    _eit_options(ts::EITOption::GEN_ALL),
    _eit_gen(_duck, ts::PID_EIT, _eit_options, ts::EITRepetitionProfile::SatelliteCable)
{
    // Connect this object as command handler for all commands.
    _opt.commands.setCommandLineHandler(this);
}


//----------------------------------------------------------------------------
// Get full path of an input or output directory.
//----------------------------------------------------------------------------

ts::UString Database::fileName(const ts::UString& directory, const ts::UString& name) const
{
    if (directory.empty() || name.empty() || name == u"-" || ts::IsAbsoluteFilePath(name)) {
        return name;
    }
    else {
        return directory + ts::PathSeparator + name;
    }
}


//----------------------------------------------------------------------------
// Database command handler, implementation of CommandLineHandlerInterface.
//----------------------------------------------------------------------------

ts::CommandStatus Database::handleCommandLine(const ts::UString& command, ts::Args& args)
{
    ts::CommandStatus status = ts::CommandStatus::SUCCESS;
    _opt.debug(u"executing command %s", {command});

    if (command == u"load") {
        ts::SectionFile file(_duck);
        if (!file.load(inputFileName(args.value(u""))) || !_eit_gen.loadEvents(file)) {
            status = ts::CommandStatus::ERROR;
        }
    }
    else if (command == u"save") {
        ts::SectionFile file(_duck);
        _eit_gen.saveEITs(file);
        if (!file.saveBinary(outputFileName(args.value(u"")))) {
            status = ts::CommandStatus::ERROR;
        }
    }
    else if (command == u"generate") {
        size_t packet_count = 0;
        if (args.present(u"bytes") + args.present(u"packets") + args.present(u"seconds") != 1) {
            args.error(u"specify exactly one of --bytes, --packets, --packets");
            status = ts::CommandStatus::ERROR;
        }
        else if (args.present(u"bytes")) {
            packet_count = args.intValue<size_t>(u"bytes") / ts::PKT_SIZE;
        }
        else if (args.present(u"packets")) {
            packet_count = args.intValue<size_t>(u"packets");
        }
        else if (_ts_bitrate == 0) {
            args.error(u"TS bitrate is unknow, --seconds cannot be used");
            status = ts::CommandStatus::ERROR;
        }
        else {
            packet_count = ts::PacketDistance(_ts_bitrate, ts::MilliSecPerSec * args.intValue<ts::Second>(u"seconds"));
        }
        if (packet_count > 0) {
            ts::TSFile file;
            ts::TSPacket pkt;
            if (!file.open(outputFileName(args.value(u"")), ts::TSFile::WRITE, args)) {
                status = ts::CommandStatus::ERROR;
            }
            else {
                while (packet_count > 0) {
                    packet_count--;
                    pkt = ts::NullPacket;
                    _eit_gen.processPacket(pkt);
                    if (!file.writePackets(&pkt, nullptr, 1, args)) {
                        break;
                    }
                }
                file.close(args);
            }
        }
    }
    else if (command == u"reset") {
        _eit_gen.reset();
    }
    else if (command == u"dump") {
        _eit_gen.dumpInternalState(ts::Severity::Info);
    }
    else if (command == u"set") {
        bool set_options = false;
        if (args.present(u"pf")) {
            _eit_options |= ts::EITOption::GEN_PF;
            set_options = true;
        }
        if (args.present(u"no-pf")) {
            _eit_options &= ~ts::EITOption::GEN_PF;
            set_options = true;
        }
        if (args.present(u"schedule")) {
            _eit_options |= ts::EITOption::GEN_SCHED;
            set_options = true;
        }
        if (args.present(u"no-schedule")) {
            _eit_options &= ~ts::EITOption::GEN_SCHED;
            set_options = true;
        }
        if (args.present(u"actual")) {
            _eit_options |= ts::EITOption::GEN_ACTUAL;
            set_options = true;
        }
        if (args.present(u"no-actual")) {
            _eit_options &= ~ts::EITOption::GEN_ACTUAL;
            set_options = true;
        }
        if (args.present(u"other")) {
            _eit_options |= ts::EITOption::GEN_OTHER;
            set_options = true;
        }
        if (args.present(u"no-other")) {
            _eit_options &= ~ts::EITOption::GEN_OTHER;
            set_options = true;
        }
        if (set_options) {
            _eit_gen.setOptions(_eit_options);
        }
        if (args.present(u"satellite")) {
            _eit_gen.setProfile(ts::EITRepetitionProfile::SatelliteCable);
        }
        if (args.present(u"terrestrial")) {
            _eit_gen.setProfile(ts::EITRepetitionProfile::Terrestrial);
        }
        if (args.present(u"ts-id")) {
            _eit_gen.setTransportStreamId(args.intValue<uint16_t>(u"ts-id"));
        }
        if (args.present(u"ts-bitrate")) {
            _ts_bitrate = args.numValue<ts::BitRate>(u"ts-bitrate");
            _eit_gen.setTransportStreamBitRate(_ts_bitrate);
        }
        if (args.present(u"eit-bitrate")) {
            _eit_gen.setMaxBitRate(args.numValue<ts::BitRate>(u"eit-bitrate"));
        }
        if (args.present(u"time")) {
            ts::Time time;
            if (!time.decode(args.value(u"time"))) {
                args.error(u"invalid --time value \"%s\" (use \"year/month/day:hour:minute:sec.ms\")", {args.value(u"time")});
                status = ts::CommandStatus::ERROR;
            }
            else {
                _eit_gen.setCurrentTime(time);
            }
        }
    }

    return status;
}


//----------------------------------------------------------------------------
// Program main code.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    ts::CommandStatus status = ts::CommandStatus::SUCCESS;

    // Get command line options.
    Options opt(argc, argv);
    Database dbase(opt);

    // Execute all command files first.
    for (size_t i = 0; status != ts::CommandStatus::EXIT && status != ts::CommandStatus::FATAL && i < opt.command_files.size(); ++i) {
        status = opt.commands.processCommandFile(opt.command_files[i]);
    }

    // Execute the direct command if present.
    if (status == ts::CommandStatus::SUCCESS && !opt.direct_command.empty()) {
        status = opt.commands.processCommand(opt.direct_command, opt.direct_args);
    }

    return (status == ts::CommandStatus::SUCCESS || status == ts::CommandStatus::EXIT) ? EXIT_SUCCESS : EXIT_FAILURE;
}
