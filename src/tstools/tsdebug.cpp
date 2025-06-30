//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSDuck debug utility. This application is not offically part of the suite
//  of TSDuck commands. It is shipped with TSDuck for troubleshooting issues
//  and as suppoort for the test suite.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsEditLine.h"
#include "tsCommandLine.h"
#include "tsZlib.h"
#include "tsNetworkInterface.h"
#include "tsIPAddress.h"
#include "tsUDPSocket.h"
#include "tsTCPServer.h"
#include "tsTelnetConnection.h"
#include "tsSysUtils.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
// Implementation of zlib commands.
//----------------------------------------------------------------------------

namespace ts {
    class ZlibCommands : public CommandLineHandler
    {
        TS_NOBUILD_NOCOPY(ZlibCommands);
    public:
        ZlibCommands(CommandLine& cmdline, int flags);
        virtual ~ZlibCommands() override;

    private:
        // Common parameters.
        bool    use_sdefl = false;
        bool    hexa_input = false;
        bool    hexa_output = false;
        UString input_file {};
        UString output_file {};

        // Load / save input / output files.
        bool loadInput(ByteBlock& input, Report& report);
        bool saveOutput(const ByteBlock& output, Report& report);

        // Common arguments.
        void defineArgs(Args& args, bool short_hexa_input);
        void loadArgs(Args& args);

        // Command handlers.
        CommandStatus compress(const UString&, Args&);
        CommandStatus decompress(const UString&, Args&);
    };
}


//----------------------------------------------------------------------------
// Zlib commands constructor and destructor.
//----------------------------------------------------------------------------

ts::ZlibCommands::ZlibCommands(CommandLine& cmdline, int flags)
{
    Args* cmd = cmdline.command(u"compress", u"Test zlib compression", u"[options]", flags);
    cmd->option(u"level", 'l', Args::INTEGER, 0, 1, 0, 9);
    cmd->help(u"level", u"Compression level. From 0 to 9. The default is 5.");
    defineArgs(*cmd, false);

    cmd = cmdline.command(u"decompress", u"Test zlib decompression", u"[options]", flags);
    defineArgs(*cmd, true);

    // Connect this object as command handler for all commands.
    cmdline.setCommandLineHandler(this, &ZlibCommands::compress, u"compress");
    cmdline.setCommandLineHandler(this, &ZlibCommands::decompress, u"decompress");
}

ts::ZlibCommands::~ZlibCommands()
{
}


//----------------------------------------------------------------------------
// Zlib commands common options.
//----------------------------------------------------------------------------

void ts::ZlibCommands::defineArgs(Args& args, bool short_hexa_input)
{
    args.option(u"hexa-input", short_hexa_input ? 'h' : 0);
    args.help(u"hexa-input", u"Interpret input file as hexa dump. Decode to binary before compressing/decompressing.");

    args.option(u"hexa-output", short_hexa_input ? 0 : 'h');
    args.help(u"hexa-output", u"Output an hexa dump of the compressed/decompressed data, instead of binary data.");

    args.option(u"input-file", 'i', Args::STRING);
    args.help(u"input-file", u"Input file name. Default to the standard input.");

    args.option(u"output-file", 'o', Args::STRING);
    args.help(u"output-file", u"Output file name. Default to the standard output.");

    args.option(u"sdefl", 's');
    args.help(u"sdefl", u"Use \"sdefl\", aka \"Small Deflate\", library. Only useful if TSDuck was compiled with zlib.");
}

void ts::ZlibCommands::loadArgs(Args& args)
{
    use_sdefl = args.present(u"sdefl");
    hexa_input = args.present(u"hexa-input");
    hexa_output = args.present(u"hexa-output");
    args.getValue(input_file, u"input-file");
    args.getValue(output_file, u"output-file");
}


//----------------------------------------------------------------------------
// Zlib commands: load / save input and output data.
//----------------------------------------------------------------------------

bool ts::ZlibCommands::loadInput(ByteBlock& input, Report& report)
{
    if (hexa_input) {
        UStringList hex;
        if (input_file.empty()) {
            UString::Load(hex, std::cin);
        }
        else if (!UString::Load(hex, input_file)) {
            report.error(u"error reading %s", input_file);
            return false;
        }
        if (!UString().join(hex).hexaDecode(input)) {
            report.error(u"invalid hexadecimal input data");
            return false;
        }
    }
    else {
        if (input_file.empty()) {
            SetBinaryModeStdin(report);
            input.read(std::cin);
        }
        else if (!input.loadFromFile(input_file, std::numeric_limits<size_t>::max(), &report)) {
            return false;
        }
    }
    report.verbose(u"input size: %d bytes", input.size());
    return true;
}

bool ts::ZlibCommands::saveOutput(const ByteBlock& output, Report& report)
{
    report.verbose(u"output size: %d bytes", output.size());
    if (hexa_output) {
        const UString hex(UString::Dump(output, UString::BPL, 0, 16));
        if (output_file.empty()) {
            std::cout << hex;
            return true;
        }
        else {
            return hex.save(output_file, false, true);
        }
    }
    else {
        if (output_file.empty()) {
            return SetBinaryModeStdout(report) && output.write(std::cout);
        }
        else {
            return output.saveToFile(output_file, &report);
        }
    }
}


//----------------------------------------------------------------------------
// Zlib commands compress and decompress.
//----------------------------------------------------------------------------

ts::CommandStatus ts::ZlibCommands::compress(const UString& command, Args& args)
{
    loadArgs(args);
    const int level = args.intValue<int>(u"level", 5);

    ByteBlock input, output;
    return loadInput(input, args) && Zlib::Compress(output, input, level, args, use_sdefl) && saveOutput(output, args) ?
        CommandStatus::SUCCESS : CommandStatus::ERROR;
}

ts::CommandStatus ts::ZlibCommands::decompress(const UString& command, Args& args)
{
    loadArgs(args);

    ByteBlock input, output;
    return loadInput(input, args) && Zlib::Decompress(output, input, args, use_sdefl) && saveOutput(output, args) ?
        CommandStatus::SUCCESS : CommandStatus::ERROR;
}


//----------------------------------------------------------------------------
// Implementation of network commands.
//----------------------------------------------------------------------------

namespace ts {
    class NetworkCommands : public CommandLineHandler
    {
        TS_NOBUILD_NOCOPY(NetworkCommands);
    public:
        NetworkCommands(CommandLine& cmdline, int flags);
        virtual ~NetworkCommands() override;

    private:
        // Common parameters.
        IP gen = IP::Any;

        // Common arguments.
        void defineArgs(Args& args);
        void loadArgs(Args& args);

        // Full image of an IP address.
        static UString Format(const IPAddress& addr);

        // Command handlers.
        CommandStatus iflist(const UString&, Args&);
        CommandStatus resolve(const UString&, Args&);
        CommandStatus receive(const UString&, Args&);
        CommandStatus send(const UString&, Args&);
    };
}


//----------------------------------------------------------------------------
// Network commands constructor and destructor.
//----------------------------------------------------------------------------

ts::NetworkCommands::NetworkCommands(CommandLine& cmdline, int flags)
{
    Args* cmd = cmdline.command(u"iflist", u"List local network interfaces", u"[options]", flags);
    cmd->option(u"no-loopback", 'n');
    cmd->help(u"no-loopback", u"Exclude loopback interfaces.");
    defineArgs(*cmd);

    cmd = cmdline.command(u"resolve", u"Resolve a network name, as in applications", u"[options] name ...", flags);
    cmd->option(u"");
    cmd->help(u"", u"Names to resolve.");
    cmd->option(u"all", 'a');
    cmd->help(u"all", u"Resolve all addresses for that name, as in nslookup.");
    defineArgs(*cmd);

    cmd = cmdline.command(u"send", u"Send a UDP or TCP message and wait for a response", u"[options] 'message-string'", flags);
    cmd->option(u"", 0, Args::STRING, 1, 1);
    cmd->help(u"", u"Message to send.");
    cmd->option(u"udp", 'u', Args::IPSOCKADDR);
    cmd->help(u"udp", u"Send the 'message-string' to the specified UDP socket and wait for a response.");
    cmd->option(u"tcp", 't', Args::IPSOCKADDR);
    cmd->help(u"tcp", u"Connect to the specified TCP server, send the 'message-string' and wait for a response.");
    defineArgs(*cmd);

    cmd = cmdline.command(u"receive", u"Receive a UDP or TCP message and send a response", u"[options]", flags);
    cmd->option(u"udp", 'u', Args::IPSOCKADDR_OA);
    cmd->help(u"udp", u"Wait for a message on the specified UDP socket and send a response.");
    cmd->option(u"tcp", 't', Args::IPSOCKADDR_OA);
    cmd->help(u"tcp", u"Create a TCP server, wait for a message and send a response.");
    defineArgs(*cmd);

    // Connect this object as command handler for all commands.
    cmdline.setCommandLineHandler(this, &NetworkCommands::iflist, u"iflist");
    cmdline.setCommandLineHandler(this, &NetworkCommands::resolve, u"resolve");
    cmdline.setCommandLineHandler(this, &NetworkCommands::receive, u"receive");
    cmdline.setCommandLineHandler(this, &NetworkCommands::send, u"send");
}

ts::NetworkCommands::~NetworkCommands()
{
}


//----------------------------------------------------------------------------
// Network commands common options.
//----------------------------------------------------------------------------

void ts::NetworkCommands::defineArgs(Args& args)
{
    args.option(u"ipv4", '4');
    args.help(u"ipv4", u"Use only IPv4 addresses.");

    args.option(u"ipv6", '6');
    args.help(u"ipv6", u"Use only IPv6 addresses.");
}

void ts::NetworkCommands::loadArgs(Args& args)
{
    if (args.present(u"ipv4")) {
        gen = IP::v4;
    }
    else if (args.present(u"ipv6")) {
        gen = IP::v6;
    }
    else {
        gen = IP::Any;
    }
}

ts::UString ts::NetworkCommands::Format(const IPAddress& addr)
{
    return UString::Format(u"%s: %s (full: \"%s\")", addr.familyName(), addr, addr.toFullString());
}


//----------------------------------------------------------------------------
// List local network interfaces.
//----------------------------------------------------------------------------

ts::CommandStatus ts::NetworkCommands::iflist(const UString& command, Args& args)
{
    loadArgs(args);
    const bool no_loopback = args.present(u"no-loopback");

    NetworkInterfaceVector net;
    if (!NetworkInterface::GetAll(net, !no_loopback, gen, false, args)) {
        return CommandStatus::ERROR;
    }

    std::cout << "Local interfaces: " << net.size() << std::endl;
    for (const auto& n : net) {
        std::cout << "  " << n << std::endl;
    }
    return CommandStatus::SUCCESS;
}


//----------------------------------------------------------------------------
// Resolve a network name or address.
//----------------------------------------------------------------------------

ts::CommandStatus ts::NetworkCommands::resolve(const UString& command, Args& args)
{
    loadArgs(args);
    const bool all = args.present(u"all");
    UStringVector names;
    args.getValues(names, u"");

    auto status = CommandStatus::SUCCESS;
    if (all) {
        // Resolve all addresses for one host name.
        for (const auto& name : names) {
            IPAddressVector addr;
            if (IPAddress::ResolveAllAddresses(addr, name, args, gen)) {
                std::cout << "Resolve \"" << name << "\":" << std::endl;
                for (const auto& a : addr) {
                    std::cout << "  " << Format(a) << std::endl;
                }
            }
            else {
                status = CommandStatus::ERROR;
            }
        }
    }
    else {
        // Resolve one host name.
        for (const auto& name : names) {
            IPAddress addr;
            if (addr.resolve(name, args, gen)) {
                std::cout << "Resolve \"" << name << "\":" << std::endl;
                std::cout << "  " << Format(addr) << std::endl;
            }
            else {
                status = CommandStatus::ERROR;
            }
        }
    }
    return status;
}


//----------------------------------------------------------------------------
// Send a TCP or UDP message.
//----------------------------------------------------------------------------

ts::CommandStatus ts::NetworkCommands::send(const UString& command, Args& args)
{
    loadArgs(args);
    const UString message(args.value(u""));
    const IPSocketAddress destination(args.socketValue(u"tcp", args.socketValue(u"udp")));

    if (args.present(u"udp") + args.present(u"tcp") != 1 || !destination.hasAddress()) {
        args.error(u"specify exactly one of --tcp and --udp");
        return CommandStatus::ERROR;
    }

    auto status = CommandStatus::SUCCESS;
    if (args.present(u"udp")) {
        // Send a UDP message.
        UDPSocket sock;
        if (!sock.open(gen, args)) {
            return CommandStatus::ERROR;
        }
        args.info(u"Sending to UDP socket %s ...", destination);
        std::string msg(message.toUTF8());
        if (sock.bind(IPSocketAddress::AnySocketAddress(gen), args) &&
            sock.send(msg.data(), msg.size(), destination, args))
        {
            size_t ret_size = 0;
            IPSocketAddress source, dest;
            msg.resize(8192);
            if (sock.receive(msg.data(), msg.size(), ret_size, source, dest, nullptr, args)) {
                msg.resize(ret_size);
                args.info(u"Received %d bytes: \"%s\"", ret_size, msg);
                args.info(u"Source: %s, destination: %s", source, dest);
            }
        }
        else {
            status = CommandStatus::ERROR;
        }
        sock.close(args);
    }
    else {
        // Send a TCP message.
        TelnetConnection client;
        if (!client.open(gen, args)) {
            return CommandStatus::ERROR;
        }
        args.info(u"Sending to TCP server %s ...", destination);
        std::string msg(message.toUTF8());
        ts::IPSocketAddress addr;
        if (client.bind(IPSocketAddress::AnySocketAddress(gen), args) &&
            client.connect(destination, args) &&
            client.getLocalAddress(addr, args) &&
            client.sendLine(msg, args) &&
            client.receiveLine(msg, nullptr, args))
        {
            args.info(u"Client address: %s", addr);
            args.info(u"Received line: \"%s\"", msg);
        }
        else {
            status = CommandStatus::ERROR;
        }
        client.close(args);
    }
    return status;
}


//----------------------------------------------------------------------------
// Receive a TCP or UDP message.
//----------------------------------------------------------------------------

ts::CommandStatus ts::NetworkCommands::receive(const UString& command, Args& args)
{
    loadArgs(args);
    const IPSocketAddress local(args.socketValue(u"tcp", args.socketValue(u"udp")));

    if (args.present(u"udp") + args.present(u"tcp") != 1 || !local.hasPort()) {
        args.error(u"specify exactly one of --tcp and --udp");
        return CommandStatus::ERROR;
    }

    auto status = CommandStatus::SUCCESS;
    if (args.present(u"udp")) {
        // Receive a UDP message, send a response.
        UDPSocket sock;
        if (!sock.open(gen, args)) {
            return CommandStatus::ERROR;
        }
        args.info(u"Waiting on UDP socket %s ...", local);
        std::string msg(8192, '\0');
        size_t ret_size = 0;
        IPSocketAddress source, dest;
        if (sock.reusePort(true, args) &&
            sock.bind(local, args) &&
            sock.receive(msg.data(), msg.size(), ret_size, source, dest, nullptr, args))
        {
            msg.resize(ret_size);
            args.info(u"Received %d bytes: \"%s\"", ret_size, msg);
            args.info(u"Source: %s, destination: %s", source, dest);
            msg.insert(0, "-> [");
            msg.append("]");
            sock.send(msg.data(), msg.size(), source, args);
        }
        else {
            status = CommandStatus::ERROR;
        }
        sock.close(args);
    }
    else {
        // TCP server, wait for a client, wait for a message, send a response.
        TCPServer server;
        if (!server.open(gen, args) ||
            !server.reusePort(true, args) ||
            !server.bind(local, args) ||
            !server.listen(1, args))
        {
            return CommandStatus::ERROR;
        }
        args.info(u"Waiting on TCP server %s ...", local);
        TelnetConnection client;
        IPSocketAddress addr;
        if (server.accept(client, addr, args)) {
            args.info(u"Client connected from %s ...", addr);
            std::string msg;
            if (client.receiveLine(msg, nullptr, args)) {
                args.info(u"Received line: \"%s\"", msg);
                msg.insert(0, "-> [");
                msg.append("]");
                client.sendLine(msg, args);
            }
            client.close();
        }
        else {
            status = CommandStatus::ERROR;
        }
        server.close(args);
    }
    return status;
}


//----------------------------------------------------------------------------
// Main command line options.
//----------------------------------------------------------------------------

namespace ts {
    class DebugCommandOptions: public Args
    {
        TS_NOBUILD_NOCOPY(DebugCommandOptions);
    public:
        DebugCommandOptions(int argc, char *argv[]);
        virtual ~DebugCommandOptions() override;

        UString       command {};       // Optional command to execute..
        UStringVector arguments {};     // Arguments of the command.
        CommandLine   cmdline {*this};  // Command line dispatcher.

    private:
        // Internal subcommands.
        static constexpr int flags = Args::NO_VERBOSE;
        ZlibCommands zlib {cmdline, flags};
        NetworkCommands net {cmdline, flags};

        // Inherited methods.
        virtual UString getHelpText(HelpFormat format, size_t line_width = DEFAULT_LINE_WIDTH) const override;
    };
}

// Constructor: get command line options.
ts::DebugCommandOptions::DebugCommandOptions(int argc, char *argv[]) :
    Args(u"TSDuck troubleshooting utility", u"[options] [command args ...]", GATHER_PARAMETERS)
{
    setIntro(u"This application is not offically part of the suite of TSDuck commands. "
             u"It is shipped with TSDuck for troubleshooting issues and as suppoort for the test suite.");

    // Command line options.
    option(u"");
    help(u"", u"Specify a subcommand. If omitted, an interactive session is started.");

    // Add internal subcommands.
    cmdline.addPredefinedCommands();

    // Analyze the command.
    analyze(argc, argv);

    // Get subcommand from the main command line.
    getValues(arguments, u"");
    if (!arguments.empty()) {
        command = arguments.front();
        arguments.erase(arguments.begin());
    }

    // Final checking
    exitOnError();
}

// Destructor.
ts::DebugCommandOptions::~DebugCommandOptions()
{
}

// Build full help text.
ts::UString ts::DebugCommandOptions::getHelpText(HelpFormat format, size_t line_width) const
{
    // Initial text from superclass.
    UString text(Args::getHelpText(format, line_width));

    // If full help, add help for all commands.
    if (format == HELP_FULL) {
        text.append(u"\nSubcommands:\n");
        const size_t margin = line_width > 10 ? 2 : 0;
        text.append(cmdline.getAllHelpText(HELP_FULL, line_width - margin).toIndented(margin));
    }
    return text;
}


//----------------------------------------------------------------------------
// Program main code.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Set defaults for interactive sessions.
    ts::EditLine::setDefaultPrompt(u"tsdebug> ");
    ts::EditLine::setDefaultNextPrompt(u">>> ");

    // Get command line options.
    ts::DebugCommandOptions opt(argc, argv);
    CERR.setMaxSeverity(opt.maxSeverity());

    // Execute one command or an interactive session.
    const ts::CommandStatus status = opt.command.empty() ?
        opt.cmdline.processInteractive(false) :
        opt.cmdline.processCommand(opt.command, opt.arguments);
    return (status == ts::CommandStatus::SUCCESS || status == ts::CommandStatus::EXIT) ? EXIT_SUCCESS : EXIT_FAILURE;
}
