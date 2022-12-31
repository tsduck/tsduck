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
//
//  EIT manipulation tool.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsEditLine.h"
#include "tsCommandLine.h"
#include "tsEITGenerator.h"
#include "tsTSFile.h"
#include "tsFileUtils.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
// Command line options
//----------------------------------------------------------------------------

namespace ts {
    class EITMainOptions: public Args
    {
        TS_NOBUILD_NOCOPY(EITMainOptions);
    public:
        EITMainOptions(int argc, char *argv[]);
        virtual ~EITMainOptions() override;

        bool          exit_error;
        UStringVector commands;
        UStringVector command_files;
        UString       input_directory;
        UString       output_directory;
        CommandLine   cmdline;

        // Inherited methods.
        virtual UString getHelpText(HelpFormat format, size_t line_width = DEFAULT_LINE_WIDTH) const override;
    };
}

// Constructor: get command line options.
ts::EITMainOptions::EITMainOptions(int argc, char *argv[]) :
    Args(u"Manipulate EIT's through commands", u"[options]"),
    exit_error(false),
    commands(),
    command_files(),
    input_directory(),
    output_directory(),
    cmdline(*this)
 {
    // Command line options.
    option(u"command", 'c', STRING, 0, UNLIMITED_COUNT);
    help(u"command", u"'string'",
         u"Specify an EIT manipulation command. "
         u"Several --command options can be specified. "
         u"All commands are executed in sequence. ");

    option(u"exit-on-error", 'e');
    help(u"exit-on-error",
         u"Stop executing commands when an error is encountered. "
         u"By default, continue execution on error.");

    option(u"file", 'f', FILENAME, 0, UNLIMITED_COUNT);
    help(u"file", u"filename",
         u"Specify a text file containing EIT manipulation commands to execute. "
         u"If the file name is empty or \"-\", the standard input is used. "
         u"Several --file options can be specified. "
         u"All files are executed in sequence. "
         u"The commands from --file are executed first, then the --command. "
         u"By default, if there no --file and no --command, commands are read from the standard input.");

    option(u"input-directory", 'i', DIRECTORY);
    help(u"input-directory",
         u"Default directory of input files in EIT manipulation commands.");

    option(u"output-directory", 'o', DIRECTORY);
    help(u"output-directory",
         u"Default directory of output files in EIT manipulation commands.");

    // EIT manipulation commands.
    Args* cmd = nullptr;
    const int flags = Args::NO_VERBOSE;

    cmdline.addPredefinedCommands();

    cmd = cmdline.command(u"load", u"Load events from a file", u"filename", flags);
    cmd->option(u"", 0, FILENAME, 1, 1);
    cmd->help(u"", u"A binary, XML or JSON file containing EIT sections.");

    cmd = cmdline.command(u"save", u"Save all current EIT sections in a file", u"filename", flags);
    cmd->option(u"", 0, FILENAME, 1, 1);
    cmd->help(u"", u"Name of the output file receiving EIT sections in binary format.");

    cmd = cmdline.command(u"process", u"Process a TS file with EIT generation", u"[options] infile outfile", flags);
    cmd->option(u"", 0, FILENAME, 2, 2);
    cmd->help(u"", u"Name of the input and output TS files.");
    cmd->option(u"start-offset", 'o', UNSIGNED);
    cmd->help(u"start-offset", u"Start offset in bytes in the input file.");
    cmd->option(u"repeat", 'r', POSITIVE);
    cmd->help(u"repeat", u"Repeat the input file the specified number of times.");
    cmd->option(u"infinite", 'i');
    cmd->help(u"infinite", u"Repeat the input file infinitely.");
    cmd->option(u"bytes", 'b', POSITIVE);
    cmd->help(u"bytes", u"Size of the TS file in bytes.");
    cmd->option(u"packets", 'p', POSITIVE);
    cmd->help(u"packets", u"Number of TS packets to generate.");
    cmd->option(u"seconds", 's', POSITIVE);
    cmd->help(u"seconds", u"Duration in seconds of the file to generate.");
    cmd->option(u"until", 'u', STRING);
    cmd->help(u"until", u"year/month/day:hour:minute:second.millisecond", u"Process up to the specified date in the stream.");

    cmd = cmdline.command(u"generate", u"Generate TS packets", u"[options] filename", flags);
    cmd->option(u"", 0, FILENAME, 1, 1);
    cmd->help(u"", u"Name of the output TS file to generate.");
    cmd->option(u"bytes", 'b', POSITIVE);
    cmd->help(u"bytes", u"Size of the TS file in bytes.");
    cmd->option(u"packets", 'p', POSITIVE);
    cmd->help(u"packets", u"Number of TS packets to generate.");
    cmd->option(u"seconds", 's', POSITIVE);
    cmd->help(u"seconds", u"Duration in seconds of the file to generate.");
    cmd->option(u"until", 'u', STRING);
    cmd->help(u"until", u"year/month/day:hour:minute:second.millisecond", u"Process up to the specified date in the stream.");

    cmdline.command(u"reset", u"Reset the content of the EIT database", u"", flags);

    cmdline.command(u"dump", u"Dump the content of the EIT database", u"", flags);

    cmd = cmdline.command(u"set", u"Set EIT generation options", u"[options]", flags);
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
    cmd->option<BitRate>(u"ts-bitrate");
    cmd->help(u"ts-bitrate", u"Set the transport stream bitrate in bits/second.");
    cmd->option<BitRate>(u"eit-bitrate");
    cmd->help(u"eit-bitrate", u"Set the EIT maximum bitrate in bits/second.");
    cmd->option(u"time", 0, STRING);
    cmd->help(u"time", u"year/month/day:hour:minute:second.millisecond", u"Set the current time.");

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    exit_error = present(u"exit-on-error");
    getValues(commands, u"command");
    getValues(command_files, u"file");
    getValue(input_directory, u"input-directory");
    getValue(output_directory, u"output-directory");

    // Final checking
    exitOnError();
}

// Destructor.
ts::EITMainOptions::~EITMainOptions()
{
}

// Build full help text.
ts::UString ts::EITMainOptions::getHelpText(HelpFormat format, size_t line_width) const
{
    // Initial text from superclass.
    UString text(Args::getHelpText(format, line_width));

    // If full help, add help for all commands.
    if (format == HELP_FULL) {
        text.append(u"\nEIT manipulation commands:\n");
        const size_t margin = line_width > 10 ? 2 : 0;
        text.append(cmdline.getAllHelpText(HELP_FULL, line_width - margin).toIndented(margin));
    }
    return text;
}


//----------------------------------------------------------------------------
// A class to manipulate the EIT database.
//----------------------------------------------------------------------------

namespace ts {
    class EITCommand : public CommandLineHandler
    {
        TS_NOBUILD_NOCOPY(EITCommand);
    public:
        EITCommand(EITMainOptions& opt);
        virtual ~EITCommand() override;

    private:
        EITMainOptions&  _opt;
        DuckContext  _duck;
        BitRate      _ts_bitrate;
        EITOptions   _eit_options;
        EITGenerator _eit_gen;

        // Get full path of an input or output directory.
        UString fileName(const UString& directory, const UString& name) const;
        UString inputFileName(const UString& name) const { return fileName(_opt.input_directory, name); }
        UString outputFileName(const UString& name) const { return fileName(_opt.output_directory, name); }

        // Get an optional time option. Return false on error. Set to Epoch if unspecified.
        bool getTimeOptions(Time& time, Args& args, const UChar* name);

        // Get processing duration options. Return false on error. Set to zero and Epoch if unspecified.
        bool getDurationOptions(size_t& packet_count, Time& until, Args& args);

        // Command handlers.
        CommandStatus load(const UString&, Args&);
        CommandStatus save(const UString&, Args&);
        CommandStatus process(const UString&, Args&);
        CommandStatus generate(const UString&, Args&);
        CommandStatus reset(const UString&, Args&);
        CommandStatus dump(const UString&, Args&);
        CommandStatus set(const UString&, Args&);
    };
}

// Destructor.
ts::EITCommand::~EITCommand()
{
}


//----------------------------------------------------------------------------
// EIT database manipulation constructor.
//----------------------------------------------------------------------------

ts::EITCommand::EITCommand(EITMainOptions& opt) :
    _opt(opt),
    _duck(&_opt),
    _ts_bitrate(0),
    _eit_options(EITOptions::GEN_ALL | EITOptions::LOAD_INPUT),
    _eit_gen(_duck, PID_EIT, _eit_options, EITRepetitionProfile::SatelliteCable)
{
    // Connect this object as command handler for all commands.
    _opt.cmdline.setCommandLineHandler(this, &EITCommand::load, u"load");
    _opt.cmdline.setCommandLineHandler(this, &EITCommand::save, u"save");
    _opt.cmdline.setCommandLineHandler(this, &EITCommand::process, u"process");
    _opt.cmdline.setCommandLineHandler(this, &EITCommand::generate, u"generate");
    _opt.cmdline.setCommandLineHandler(this, &EITCommand::reset, u"reset");
    _opt.cmdline.setCommandLineHandler(this, &EITCommand::dump, u"dump");
    _opt.cmdline.setCommandLineHandler(this, &EITCommand::set, u"set");
}


//----------------------------------------------------------------------------
// Get full path of an input or output directory.
//----------------------------------------------------------------------------

ts::UString ts::EITCommand::fileName(const UString& directory, const UString& name) const
{
    if (directory.empty() || name.empty() || name == u"-" || IsAbsoluteFilePath(name)) {
        return name;
    }
    else {
        return directory + PathSeparator + name;
    }
}


//----------------------------------------------------------------------------
// Get an optional time option.
//----------------------------------------------------------------------------

bool ts::EITCommand::getTimeOptions(Time& time, Args& args, const UChar* name)
{
    time = Time::Epoch;
    if (!args.present(name) || time.decode(args.value(name), Time::ALL)) {
        return true;
    }
    else {
        args.error(u"invalid --%s value \"%s\" (use \"year/month/day:hour:minute:sec.ms\")", {name, args.value(name)});
        return false;
    }
}


//----------------------------------------------------------------------------
// Get processing duration options.
//----------------------------------------------------------------------------

bool ts::EITCommand::getDurationOptions(size_t& packet_count, Time& until, Args& args)
{
    packet_count = 0;

    if (_ts_bitrate == 0 && (args.present(u"until") || args.present(u"seconds"))) {
        args.error(u"TS bitrate is unknow, --until or --seconds cannot be used");
        return false;
    }
    if (!getTimeOptions(until, args, u"until")) {
        return false;
    }
    if (args.present(u"bytes") + args.present(u"packets") + args.present(u"seconds") > 1) {
        args.error(u"specify at most one of --bytes, --packets, --seconds");
        return false;
    }
    if (args.present(u"bytes")) {
        packet_count = args.intValue<size_t>(u"bytes") / PKT_SIZE;
    }
    else if (args.present(u"packets")) {
        packet_count = args.intValue<size_t>(u"packets");
    }
    else if (args.present(u"seconds")) {
        packet_count = size_t(PacketDistance(_ts_bitrate, MilliSecPerSec * args.intValue<Second>(u"seconds")));
    }
    return true;
}


//----------------------------------------------------------------------------
// Database command handler, implementation of CommandLineHandlerInterface.
//----------------------------------------------------------------------------

ts::CommandStatus ts::EITCommand::load(const UString& command, Args& args)
{
    SectionFile file(_duck);
    if (file.load(inputFileName(args.value(u""))) && _eit_gen.loadEvents(file)) {
        return CommandStatus::SUCCESS;
    }
    else {
        return CommandStatus::ERROR;
    }
}

ts::CommandStatus ts::EITCommand::save(const UString& command, Args& args)
{
    SectionFile file(_duck);
    _eit_gen.saveEITs(file);
    return file.saveBinary(outputFileName(args.value(u""))) ? CommandStatus::SUCCESS : CommandStatus::ERROR;
}

ts::CommandStatus ts::EITCommand::process(const UString& command, Args& args)
{
    const UString infile_name(inputFileName(args.value(u"", u"", 0)));
    const UString outfile_name(outputFileName(args.value(u"", u"", 1)));
    const uint64_t start_offset = args.intValue<uint64_t>(u"start-offset", 0);
    const size_t repeat_count = args.intValue<size_t>(u"repeat", args.present(u"infinite") ? 0 : 1);
    size_t packet_count = 0;
    Time until;
    TSFile infile;
    TSFile outfile;
    TSPacket pkt;

    if (!getDurationOptions(packet_count, until, args) ||
        !infile.openRead(infile_name, repeat_count, start_offset, args) ||
        !outfile.open(outfile_name, TSFile::WRITE, args))
    {
        return CommandStatus::ERROR;
    }

    for (size_t count = 0;
         (packet_count == 0 || count < packet_count) && (until == Time::Epoch || _eit_gen.getCurrentTime() < until) && infile.readPackets(&pkt, nullptr, 1, args);
         ++count)
    {
        _eit_gen.processPacket(pkt);
        if (!outfile.writePackets(&pkt, nullptr, 1, args)) {
            return CommandStatus::ERROR;
        }
    }
    return CommandStatus::SUCCESS;
}

ts::CommandStatus ts::EITCommand::generate(const UString& command, Args& args)
{
    size_t packet_count = 0;
    Time until;

    if (!getDurationOptions(packet_count, until, args)) {
        return CommandStatus::ERROR;
    }

    if (packet_count == 0 && until == Time::Epoch) {
        args.error(u"no size or duration specified");
        return CommandStatus::ERROR;
    }

    TSFile file;
    if (!file.open(outputFileName(args.value(u"")), TSFile::WRITE, args)) {
        return CommandStatus::ERROR;
    }

    TSPacket pkt;
    for (size_t count = 0; (packet_count == 0 || count < packet_count) && (until == Time::Epoch || _eit_gen.getCurrentTime() < until); ++count) {
        pkt = NullPacket;
        _eit_gen.processPacket(pkt);
        if (!file.writePackets(&pkt, nullptr, 1, args)) {
            return CommandStatus::ERROR;
        }
    }
    return CommandStatus::SUCCESS;
}

ts::CommandStatus ts::EITCommand::reset(const UString& command, Args& args)
{
    _eit_gen.reset();
    return CommandStatus::SUCCESS;
}

ts::CommandStatus ts::EITCommand::dump(const UString& command, Args& args)
{
    _eit_gen.dumpInternalState(Severity::Info);
    return CommandStatus::SUCCESS;
}

ts::CommandStatus ts::EITCommand::set(const UString& command, Args& args)
{
    bool set_options = false;
    if (args.present(u"pf")) {
        _eit_options |= EITOptions::GEN_PF;
        set_options = true;
    }
    if (args.present(u"no-pf")) {
        _eit_options &= ~EITOptions::GEN_PF;
        set_options = true;
    }
    if (args.present(u"schedule")) {
        _eit_options |= EITOptions::GEN_SCHED;
        set_options = true;
    }
    if (args.present(u"no-schedule")) {
        _eit_options &= ~EITOptions::GEN_SCHED;
        set_options = true;
    }
    if (args.present(u"actual")) {
        _eit_options |= EITOptions::GEN_ACTUAL;
        set_options = true;
    }
    if (args.present(u"no-actual")) {
        _eit_options &= ~EITOptions::GEN_ACTUAL;
        set_options = true;
    }
    if (args.present(u"other")) {
        _eit_options |= EITOptions::GEN_OTHER;
        set_options = true;
    }
    if (args.present(u"no-other")) {
        _eit_options &= ~EITOptions::GEN_OTHER;
        set_options = true;
    }
    if (set_options) {
        _eit_gen.setOptions(_eit_options);
    }

    Time time;
    if (!getTimeOptions(time, args, u"time")) {
        return CommandStatus::ERROR;
    }
    if (time != Time::Epoch) {
        _eit_gen.setCurrentTime(time);
    }

    if (args.present(u"satellite")) {
        _eit_gen.setProfile(EITRepetitionProfile::SatelliteCable);
    }
    if (args.present(u"terrestrial")) {
        _eit_gen.setProfile(EITRepetitionProfile::Terrestrial);
    }
    if (args.present(u"ts-id")) {
        _eit_gen.setTransportStreamId(args.intValue<uint16_t>(u"ts-id"));
    }
    if (args.present(u"ts-bitrate")) {
        _ts_bitrate = args.numValue<BitRate>(u"ts-bitrate");
        _eit_gen.setTransportStreamBitRate(_ts_bitrate);
    }
    if (args.present(u"eit-bitrate")) {
        _eit_gen.setMaxBitRate(args.numValue<BitRate>(u"eit-bitrate"));
    }

    return CommandStatus::SUCCESS;
}


//----------------------------------------------------------------------------
// Program main code.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Set defaults for interactive sessions.
    ts::EditLine::setDefaultPrompt(u"tseit> ");
    ts::EditLine::setDefaultNextPrompt(u">>> ");

    // Get command line options.
    ts::EITMainOptions opt(argc, argv);
    ts::EITCommand dbase(opt);

    ts::CommandStatus status = ts::CommandStatus::SUCCESS;
    if (opt.command_files.empty() && opt.commands.empty()) {
        // Interactive session.
        status = opt.cmdline.processInteractive(opt.exit_error);
    }
    else {
        // Execute all --file first, then all --command.
        status = opt.cmdline.processCommandFiles(opt.command_files, opt.exit_error);
        if (status == ts::CommandStatus::SUCCESS || (status == ts::CommandStatus::ERROR && !opt.exit_error)) {
            status = opt.cmdline.processCommands(opt.commands, opt.exit_error);
        }
    }
    return (status == ts::CommandStatus::SUCCESS || status == ts::CommandStatus::EXIT) ? EXIT_SUCCESS : EXIT_FAILURE;
}
