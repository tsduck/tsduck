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
//  and as support for the test suite.
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
#include "tsTLSServer.h"
#include "tsRestServer.h"
#include "tsTelnetConnection.h"
#include "tsWebRequest.h"
#include "tsTLSArgs.h"
#include "tsSysUtils.h"
#include "tsIPUtils.h"
#include "tsjsonValue.h"
#if defined(TS_WINDOWS)
    #include "tsWinUtils.h"
    #include "tsWinModuleInfo.h"
#endif
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
// Error message commands.
//----------------------------------------------------------------------------

namespace ts {
    class ErrorCommands: public CommandLineHandler
    {
        TS_NOBUILD_NOCOPY(ErrorCommands);
    public:
        ErrorCommands(CommandLine& cmdline, int flags);
        virtual ~ErrorCommands() override;

    private:
        // Command line parameters for --category.
        enum Category {SYSTEM, GETADDRINFO};
        const Names _category_names {
            {u"system",      SYSTEM},
            {u"getaddrinfo", GETADDRINFO},
        };
        const std::map<Category, const std::error_category*> _categories {
            {SYSTEM,      &std::system_category()},
            {GETADDRINFO, &ts::getaddrinfo_category()},
        };

        // Command handlers.
        CommandStatus error(const UString&, Args&);
    };
}

ts::ErrorCommands::ErrorCommands(CommandLine& cmdline, int flags)
{
    Args* cmd = cmdline.command(u"error", u"Interpret system error code", u"[options] code", flags);
    cmdline.setCommandLineHandler(this, &ErrorCommands::error, u"error");
    cmd->option(u"", 0, Args::UINT32);
    cmd->help(u"", u"Error code values.");
    cmd->option(u"category", 'c', _category_names);
    cmd->help(u"category", u"C++ category (std::error_category).");
    cmd->option(u"windows", 'w');
    cmd->help(u"windows", u"On Windwos, use Win32 functions instead of C++ standard functions.");
}

ts::ErrorCommands::~ErrorCommands()
{
}

ts::CommandStatus ts::ErrorCommands::error(const UString& command, Args& args)
{
    std::vector<int> codes;
    args.getIntValues(codes, u"");

    const auto category = _categories.find(args.intValue<Category>(u"category", SYSTEM));
    if (category == _categories.end()) {
        args.error(u"invalid category");
        return CommandStatus::ERROR;
    }

#if defined(TS_WINDOWS)
    const bool win = args.present(u"windows");
#else
    constexpr bool win = false;
#endif

    for (auto code : codes) {
        UString message;
        if (win) {
#if defined(TS_WINDOWS)
            message = WinErrorMessage(code);
#endif
        }
        else {
            message = UString(SysErrorCodeMessage(code, *category->second));
        }
        std::cout << UString::Format(u"%X: \"%s\"", code, message) << std::endl;
    }

    return CommandStatus::SUCCESS;
}


//----------------------------------------------------------------------------
// Windows module information commands.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)
namespace ts {
    class WinModuleCommands: public CommandLineHandler
    {
        TS_NOBUILD_NOCOPY(WinModuleCommands);
    public:
        WinModuleCommands(CommandLine& cmdline, int flags);
        virtual ~WinModuleCommands() override;

    private:
        void displayInt(const UString& name, size_t width, uint64_t value);
        // Command handlers.
        CommandStatus moduleInfo(const UString&, Args&);
    };
}
#endif


//----------------------------------------------------------------------------
// Windows module information commands constructor and destructor.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)
ts::WinModuleCommands::WinModuleCommands(CommandLine& cmdline, int flags)
{
    Args* cmd = cmdline.command(u"module", u"Display information of a Windows module file", u"[options] file", flags);
    cmdline.setCommandLineHandler(this, &WinModuleCommands::moduleInfo, u"module");
    cmd->option(u"", 0, Args::FILENAME, 1, 1);
    cmd->help(u"", u"Module file name (DLL or executable).");
}

ts::WinModuleCommands::~WinModuleCommands()
{
}
#endif


//----------------------------------------------------------------------------
// Windows module information command.
//----------------------------------------------------------------------------

#if defined(TS_WINDOWS)
ts::CommandStatus ts::WinModuleCommands::moduleInfo(const UString& command, Args& args)
{
    WinModuleInfo info(args.value(u""));
    if (info.isValid()) {
        static const UString file_header(u"File version");
        static const UString product_header(u"Product version");

        // Max display size of names.
        size_t max_width = std::max(file_header.size(), product_header.size());
        for (const auto& it : WinModuleInfo::GetNames()) {
            max_width = std::max(max_width, it.second.size());
        }

        // Display values.
        displayInt(file_header, max_width, info.file_version_int);
        displayInt(product_header, max_width, info.product_version_int);
        for (const auto& it : WinModuleInfo::GetNames()) {
            std::cout << UString::Format(u"%-*s  \"%s\"", max_width, it.second, info.*(it.first)) << std::endl;
        }
        std::cout << UString::Format(u"%-*s  \"%s\"", max_width, u"Summary", info.summary()) << std::endl;

        return CommandStatus::SUCCESS;
    }
    else {
        args.error(info.lastError());
        return CommandStatus::ERROR;
    }
}

void ts::WinModuleCommands::displayInt(const UString& name, size_t width, uint64_t value)
{
    std::cout << UString::Format(u"%-*s  0x%X (%d.%d.%d.%d)",
                                 width, name, value,
                                 value >> 48, (value >> 32) & 0xFFFF, (value >> 16) & 0xFFFF, value & 0xFFFF) 
              << std::endl;
}
#endif


//----------------------------------------------------------------------------
// Zlib commands.
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

ts::ZlibCommands::ZlibCommands(CommandLine& cmdline, int flags)
{
    Args* cmd = cmdline.command(u"compress", u"Test zlib compression", u"[options]", flags);
    cmdline.setCommandLineHandler(this, &ZlibCommands::compress, u"compress");
    defineArgs(*cmd, false);
    cmd->option(u"level", 'l', Args::INTEGER, 0, 1, 0, 9);
    cmd->help(u"level", u"Compression level. From 0 to 9. The default is 5.");

    cmd = cmdline.command(u"decompress", u"Test zlib decompression", u"[options]", flags);
    cmdline.setCommandLineHandler(this, &ZlibCommands::decompress, u"decompress");
    defineArgs(*cmd, true);
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
// Base class for network commands, defining coommon IP options.
//----------------------------------------------------------------------------

namespace ts {
    class NetworkBase
    {
    protected:
        // Common parameters.
        IP ip_gen = IP::Any;

        // Common arguments.
        void defineIPGenArgs(Args& args);
        void loadIPGenArgs(Args& args);

        // Full image of an IP address.
        static UString Format(const IPAddress& addr);
    };
}

void ts::NetworkBase::defineIPGenArgs(Args& args)
{
    args.option(u"ipv4", '4');
    args.help(u"ipv4", u"Use only IPv4 addresses.");

    args.option(u"ipv6", '6');
    args.help(u"ipv6", u"Use only IPv6 addresses.");
}

void ts::NetworkBase::loadIPGenArgs(Args& args)
{
    if (args.present(u"ipv4")) {
        ip_gen = IP::v4;
    }
    else if (args.present(u"ipv6")) {
        ip_gen = IP::v6;
    }
    else {
        ip_gen = IP::Any;
    }
}

ts::UString ts::NetworkBase::Format(const IPAddress& addr)
{
    return UString::Format(u"%s: %s (full: \"%s\")", addr.familyName(), addr, addr.toFullString());
}


//----------------------------------------------------------------------------
// Network commands.
//----------------------------------------------------------------------------

namespace ts {
    class NetworkCommands : public CommandLineHandler, protected NetworkBase
    {
        TS_NOBUILD_NOCOPY(NetworkCommands);
    public:
        NetworkCommands(CommandLine& cmdline, int flags);
        virtual ~NetworkCommands() override;

    private:
        // Command handlers.
        CommandStatus iflist(const UString&, Args&);
        CommandStatus resolve(const UString&, Args&);
    };
}

ts::NetworkCommands::NetworkCommands(CommandLine& cmdline, int flags)
{
    Args* cmd = cmdline.command(u"iflist", u"List local network interfaces", u"[options]", flags);
    cmdline.setCommandLineHandler(this, &NetworkCommands::iflist, u"iflist");
    defineIPGenArgs(*cmd);
    cmd->option(u"no-loopback", 'n');
    cmd->help(u"no-loopback", u"Exclude loopback interfaces.");

    cmd = cmdline.command(u"resolve", u"Resolve a network name, as in applications", u"[options] name ...", flags);
    cmdline.setCommandLineHandler(this, &NetworkCommands::resolve, u"resolve");
    defineIPGenArgs(*cmd);
    cmd->option(u"");
    cmd->help(u"", u"Names to resolve.");
    cmd->option(u"all", 'a');
    cmd->help(u"all", u"Resolve all addresses for that name, as in nslookup.");
}

ts::NetworkCommands::~NetworkCommands()
{
}


//----------------------------------------------------------------------------
// List local network interfaces.
//----------------------------------------------------------------------------

ts::CommandStatus ts::NetworkCommands::iflist(const UString& command, Args& args)
{
    loadIPGenArgs(args);
    const bool no_loopback = args.present(u"no-loopback");

    NetworkInterfaceVector net;
    if (!NetworkInterface::GetAll(net, !no_loopback, ip_gen, false, args)) {
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
    loadIPGenArgs(args);
    const bool all = args.present(u"all");
    UStringVector names;
    args.getValues(names, u"");

    auto status = CommandStatus::SUCCESS;
    if (all) {
        // Resolve all addresses for one host name.
        for (const auto& name : names) {
            IPAddressVector addr;
            if (IPAddress::ResolveAllAddresses(addr, name, args, ip_gen)) {
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
            if (addr.resolve(name, args, ip_gen)) {
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
// Send / receive commands.
//----------------------------------------------------------------------------

namespace ts {
    class SendRecvCommands: public CommandLineHandler, protected NetworkBase
    {
        TS_NOBUILD_NOCOPY(SendRecvCommands);
    public:
        SendRecvCommands(CommandLine& cmdline, int flags);
        virtual ~SendRecvCommands() override;

    private:
        TLSArgs _tls_args {};
        // Command handlers.
        CommandStatus receive(const UString&, Args&);
        CommandStatus send(const UString&, Args&);
    };
}

ts::SendRecvCommands::SendRecvCommands(CommandLine& cmdline, int flags)
{
    Args* cmd = cmdline.command(u"send", u"Send a UDP or TCP message and wait for a response", u"[options] 'message-string'", flags);
    cmdline.setCommandLineHandler(this, &SendRecvCommands::send, u"send");
    defineIPGenArgs(*cmd);
    cmd->option(u"", 0, Args::STRING, 1, 1);
    cmd->help(u"", u"Message to send.");
    cmd->option(u"udp", 'u', Args::IPSOCKADDR);
    cmd->help(u"udp", u"Send the 'message-string' to the specified UDP socket and wait for a response.");
    cmd->option(u"tcp", 't', Args::IPSOCKADDR);
    cmd->help(u"tcp", u"Connect to the specified TCP server, send the 'message-string' and wait for a response.");
    _tls_args.defineClientArgs(*cmd);

    cmd = cmdline.command(u"receive", u"Receive a UDP or TCP message and send a response", u"[options]", flags);
    cmdline.setCommandLineHandler(this, &SendRecvCommands::receive, u"receive");
    defineIPGenArgs(*cmd);
    cmd->option(u"udp", 'u', Args::IPSOCKADDR_OA);
    cmd->help(u"udp", u"Wait for a message on the specified UDP socket and send a response.");
    cmd->option(u"tcp", 't', Args::IPSOCKADDR_OA);
    cmd->help(u"tcp", u"Create a TCP server, wait for a message and send a response.");
    _tls_args.defineServerArgs(*cmd);
}

ts::SendRecvCommands::~SendRecvCommands()
{
}


//----------------------------------------------------------------------------
// Send a TCP or UDP message.
//----------------------------------------------------------------------------

ts::CommandStatus ts::SendRecvCommands::send(const UString& command, Args& args)
{
    loadIPGenArgs(args);
    const bool use_udp = args.present(u"udp");
    _tls_args.loadClientArgs(args, use_udp ? u"udp" : u"tcp");
    const UString message(args.value(u""));

    if (args.present(u"udp") + args.present(u"tcp") != 1 || !_tls_args.server_addr.hasAddress()) {
        args.error(u"specify exactly one of --tcp and --udp");
        return CommandStatus::ERROR;
    }

    auto status = CommandStatus::SUCCESS;
    if (use_udp) {
        // Send a UDP message.
        UDPSocket sock;
        if (!sock.open(ip_gen, args)) {
            return CommandStatus::ERROR;
        }
        args.info(u"Sending to UDP socket %s ...", _tls_args.server_addr);
        std::string msg(message.toUTF8());
        if (sock.bind(IPSocketAddress::AnySocketAddress(ip_gen), args) &&
            sock.send(msg.data(), msg.size(), _tls_args.server_addr, args))
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
        TCPConnection tcp_client;
        TLSConnection tls_client(_tls_args);
        TCPConnection* const client = _tls_args.use_tls ? &tls_client : &tcp_client;
        TelnetConnection telnet(*client);

        if (!client->open(ip_gen, args)) {
            return CommandStatus::ERROR;
        }
        args.info(u"Sending to TCP server %s ...", _tls_args.server_addr);
        std::string msg(message.toUTF8());
        ts::IPSocketAddress addr;
        if (client->bind(IPSocketAddress::AnySocketAddress(ip_gen), args) &&
            client->connect(_tls_args.server_addr, args) &&
            client->getLocalAddress(addr, args) &&
            telnet.sendLine(msg, args) &&
            telnet.receiveLine(msg, nullptr, args))
        {
            args.info(u"Client address: %s", addr);
            args.info(u"Received line: \"%s\"", msg);
        }
        else {
            status = CommandStatus::ERROR;
        }
        client->close(args);
    }
    return status;
}


//----------------------------------------------------------------------------
// Receive a TCP or UDP message.
//----------------------------------------------------------------------------

ts::CommandStatus ts::SendRecvCommands::receive(const UString& command, Args& args)
{
    loadIPGenArgs(args);
    const bool use_udp = args.present(u"udp");
    _tls_args.loadServerArgs(args, use_udp ? u"udp" : u"tcp");

    if (args.present(u"udp") + args.present(u"tcp") != 1 || !_tls_args.server_addr.hasPort()) {
        args.error(u"specify exactly one of --tcp and --udp");
        return CommandStatus::ERROR;
    }

    auto status = CommandStatus::SUCCESS;
    if (use_udp) {
        // Receive a UDP message, send a response.
        UDPSocket sock;
        if (!sock.open(ip_gen, args)) {
            return CommandStatus::ERROR;
        }
        args.info(u"Waiting on UDP socket %s ...", _tls_args.server_addr);
        std::string msg(8192, '\0');
        size_t ret_size = 0;
        IPSocketAddress source, dest;
        if (sock.reusePort(true, args) &&
            sock.bind(_tls_args.server_addr, args) &&
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
        TCPServer tcp_server;
        TLSServer tls_server(_tls_args);
        TCPServer* const server = _tls_args.use_tls ? &tls_server : &tcp_server;

        if (!server->open(ip_gen, args) ||
            !server->reusePort(true, args) ||
            !server->bind(_tls_args.server_addr, args) ||
            !server->listen(1, args))
        {
            return CommandStatus::ERROR;
        }

        args.info(u"Waiting on TCP server %s ...", _tls_args.server_addr);
        TCPConnection tcp_client;
        TLSConnection tls_client;
        tls_client.setVerifyPeer(false);
        TCPConnection* const client = _tls_args.use_tls ? &tls_client : &tcp_client;
        TelnetConnection telnet(*client);
        IPSocketAddress addr;
        if (server->accept(*client, addr, args)) {
            args.info(u"Client connected from %s ...", addr);
            std::string msg;
            if (telnet.receiveLine(msg, nullptr, args)) {
                args.info(u"Received line: \"%s\"", msg);
                msg.insert(0, "-> [");
                msg.append("]");
                telnet.sendLine(msg, args);
            }
            client->disconnect(args);
            client->close(args);
        }
        else {
            status = CommandStatus::ERROR;
        }
        server->close(args);
    }
    return status;
}


//----------------------------------------------------------------------------
// HTTP server commands.
//----------------------------------------------------------------------------

namespace ts {
    class ServerCommands : public CommandLineHandler, protected NetworkBase
    {
        TS_NOBUILD_NOCOPY(ServerCommands);
    public:
        ServerCommands(CommandLine& cmdline, int flags);
        virtual ~ServerCommands() override;

    private:
        TLSArgs _tls_args {};
        // Command handlers.
        CommandStatus server(const UString&, Args&);
    };
}


//----------------------------------------------------------------------------
// HTTP server commands constructor and destructor.
//----------------------------------------------------------------------------

ts::ServerCommands::ServerCommands(CommandLine& cmdline, int flags)
{
    Args* cmd = cmdline.command(u"server", u"Basic HTTP server which dumps its requests", u"[options] [ip-address:]port", flags);
    cmdline.setCommandLineHandler(this, &ServerCommands::server, u"server");
    defineIPGenArgs(*cmd);
    cmd->option(u"", 0, Args::IPSOCKADDR_OA, 1, 1);
    cmd->help(u"", u"TCP server local address.");
    cmd->option(u"max-clients", 'm', Args::UNSIGNED);
    cmd->help(u"max-clients", u"Exit after this number of client sessions. By default, never exit.");
    cmd->option(u"sort-headers", 's');
    cmd->help(u"sort-headers", u"Sort request headers before displaying them. For reproducibility of tests.");
    cmd->option(u"hide-header", 'h', Args::STRING, 0, Args::UNLIMITED_COUNT);
    cmd->help(u"hide-header", u"Hide this request header from display. For reproducibility of tests.");
    _tls_args.defineServerArgs(*cmd);
}

ts::ServerCommands::~ServerCommands()
{
}


//----------------------------------------------------------------------------
// Basic HTTP server which dumps its requests.
//----------------------------------------------------------------------------

ts::CommandStatus ts::ServerCommands::server(const UString& command, Args& args)
{
    loadIPGenArgs(args);
    _tls_args.loadServerArgs(args, u"");
    const bool sort_headers = args.present(u"sort-headers");
    size_t max_clients = args.intValue<size_t>(u"max-clients", std::numeric_limits<size_t>::max());
    UStringVector hidden;
    args.getValues(hidden, u"hide-header");

    TCPServer tcp_server;
    TLSServer tls_server(_tls_args);
    TCPServer* const server = _tls_args.use_tls ? &tls_server : &tcp_server;

    if (!server->open(ip_gen, args) ||
        !server->reusePort(true, args) ||
        !server->bind(_tls_args.server_addr, args) ||
        !server->listen(16, args))
    {
        return CommandStatus::ERROR;
    }

    while (max_clients-- > 0) {
        args.verbose(u"Waiting on TCP server %s ...", _tls_args.server_addr);

        TCPConnection tcp_client;
        TLSConnection tls_client;
        tls_client.setVerifyPeer(false);
        TCPConnection* const client = _tls_args.use_tls ? &tls_client : &tcp_client;
        TelnetConnection telnet(*client);

        IPSocketAddress addr;
        if (!server->accept(*client, addr, args)) {
            // Failed to accept a client, try a new one.
            continue;
        }
        args.verbose(u"Client connected from %s ...", addr);

        // Loop on request headers.
        UString line;
        bool success = true;
        bool first_line = true;
        bool expect_data = false;
        bool is_text = false;
        size_t content_size = 0;
        UStringList headers;
        args.info(u"==== Request headers:");
        do {
            success = telnet.receiveLine(line, nullptr, args);
            // Analyze the header line.
            if (first_line) {
                args.info(line);
                expect_data = line.starts_with(u"POST") || line.starts_with(u"PUT") || line.starts_with(u"PATCH");
                first_line = false;
            }
            else {
                UStringVector fields;
                line.split(fields, u':', true, true);
                if (fields.size() >= 2) {
                    // This is a true header.
                    size_t value = 0;
                    if (fields[0].similar(u"Content-Length") && fields[1].toInteger(value)) {
                        content_size = value;
                    }
                    else if (fields[0].similar(u"Content-Type")) {
                        is_text = fields[1].contains(u"text", CASE_INSENSITIVE) ||
                                  fields[1].contains(u"json", CASE_INSENSITIVE) ||
                                  fields[1].contains(u"xml", CASE_INSENSITIVE);
                    }
                    if (!fields[0].isContainedSimilarIn(hidden)) {
                        headers.push_back(line);
                    }
                }
                else if (!line.empty()) {
                    // Not a true header, just display it.
                    headers.push_back(line);
                }
            }
        } while (success && !line.empty());

        // Display headers.
        if (sort_headers) {
            headers.sort();
        }
        for (const auto& h : headers) {
            args.info(h);
        }

        if (success) {
            // All headers are read, including final empty line.
            // Try to get PUT or POST data.
            ByteBlock data;
            telnet.getAndFlush(data);
            if (content_size > data.size()) {
                // We known how much more data we need.
                const size_t previous_size = data.size();
                data.resize(content_size);
                success = client->receive(&data[previous_size], content_size - previous_size, nullptr, args);
            }
            else if (content_size == 0 && expect_data) {
                // Unknown content size but there must be some. This is an old client which disconnects at end of request.
                for (;;) {
                    const size_t previous_size = data.size();
                    constexpr size_t more_size = 4096;
                    size_t ret_size = 0;
                    data.resize(previous_size + more_size);
                    if (client->receive(&data[previous_size], more_size, ret_size, nullptr, args)) {
                        data.resize(previous_size + ret_size);
                    }
                    else {
                        data.resize(previous_size);
                        break;
                    }
                }
            }

            // Display request data.
            if (!data.empty())  {
                args.info(u"==== Request data (%d bytes):", data.size());
                if (is_text) {
                    args.info(UString::FromUTF8(reinterpret_cast<const char*>(data.data()), data.size()));
                }
                else {
                    UString dump(UString::Dump(data, UString::HEXA | UString::ASCII | UString::BPL, 0, 16));
                    dump.trim(false, true, false);
                    args.info(dump);
                }
            }

            // Send a "no data" response.
            telnet.sendLine(u"HTTP/1.1 204 No Content", args);
            telnet.sendLine("Server: TSDuck", args);
            telnet.sendLine("Connection: close", args);
            telnet.sendLine(u"", args);
        }

        client->disconnect(args);
        client->close(args);
    }

    server->close(args);
    return CommandStatus::SUCCESS;
}


//----------------------------------------------------------------------------
// HTTP client command, using explicit TCPConnection or TLSConnection.
//----------------------------------------------------------------------------

namespace ts {
    class ClientCommands: public CommandLineHandler, protected NetworkBase
    {
        TS_NOBUILD_NOCOPY(ClientCommands);
    public:
        ClientCommands(CommandLine& cmdline, int flags);
        virtual ~ClientCommands() override;

    private:
        TLSArgs _tls_args {};
        // Command handlers.
        CommandStatus client(const UString&, Args&);
    };
}


//----------------------------------------------------------------------------
// HTTP client commands constructor and destructor.
//----------------------------------------------------------------------------

ts::ClientCommands::ClientCommands(CommandLine& cmdline, int flags)
{
    Args* cmd = cmdline.command(u"client", u"Basic HTTP client which dumps its text reponse", u"[options] ip-address:port", flags);
    cmdline.setCommandLineHandler(this, &ClientCommands::client, u"client");
    defineIPGenArgs(*cmd);
    cmd->option(u"", 0, Args::IPSOCKADDR_OP, 1, 1);
    cmd->help(u"", u"TCP server address and port.");
    cmd->option(u"header", 'h', Args::STRING, 0, Args::UNLIMITED_COUNT);
    cmd->help(u"header", u"Add this request header.");
    cmd->option(u"request", 'r', Args::STRING);
    cmd->help(u"request", u"Request line. Default: \"GET /\"");
    _tls_args.defineClientArgs(*cmd);
}

ts::ClientCommands::~ClientCommands()
{
}


//----------------------------------------------------------------------------
// Basic HTTP client which dumps its response.
//----------------------------------------------------------------------------

ts::CommandStatus ts::ClientCommands::client(const UString& command, Args& args)
{
    loadIPGenArgs(args);
    _tls_args.loadClientArgs(args, u"");
    const UString request(args.value(u"request", u"GET /"));
    UStringList headers;
    args.getValues(headers, u"header");

    // Build full input lines.
    headers.push_front(u"Accept: */*");
    headers.push_front(u"Connection: close");
    headers.push_front(u"User-Agent: TSDuck");
    headers.push_front(u"Host: " + _tls_args.server_name);
    headers.push_front(request + u" HTTP/1.1");
    headers.push_back(u"");

    auto status = CommandStatus::SUCCESS;
    TCPConnection tcp_client;
    TLSConnection tls_client(_tls_args);
    TCPConnection* const client = _tls_args.use_tls ? &tls_client : &tcp_client;
    TelnetConnection telnet(*client);

    // Connect to the server.
    if (!client->open(ip_gen, args) ||
        !client->bind(IPSocketAddress::AnySocketAddress(ip_gen), args) ||
        !client->connect(_tls_args.server_addr, args))
    {
        return CommandStatus::ERROR;
    }

    // Send all input lines.
    for (const auto& line : headers) {
        if (!telnet.sendLine(line, args)) {
            return CommandStatus::ERROR;
        }
    }

    // Receive responses.
    UString response;
    while (telnet.receiveLine(response, nullptr, args)) {
        args.info(response);
    }
    client->close(args);
    return status;
}


//----------------------------------------------------------------------------
// HTTP client command using WebRequest on URL.
//----------------------------------------------------------------------------

namespace ts {
    class URLCommands: public CommandLineHandler
    {
        TS_NOBUILD_NOCOPY(URLCommands);
    public:
        URLCommands(CommandLine& cmdline, int flags);
        virtual ~URLCommands() override;

    private:
        // Command handlers.
        CommandStatus geturl(const UString&, Args&);
    };
}


//----------------------------------------------------------------------------
// URL commands constructor and destructor.
//----------------------------------------------------------------------------

ts::URLCommands::URLCommands(CommandLine& cmdline, int flags)
{
    Args* cmd = cmdline.command(u"geturl", u"Get a URL and dump its text reponse", u"[options] url", flags);
    cmdline.setCommandLineHandler(this, &URLCommands::geturl, u"geturl");
    cmd->option(u"", 0, Args::STRING, 1, 1);
    cmd->help(u"", u"URL to get.");
    cmd->option(u"header", 'h', Args::STRING, 0, Args::UNLIMITED_COUNT);
    cmd->help(u"header", u"'name: value'", u"Add this request header.");
    cmd->option(u"insecure");
    cmd->help(u"insecure", u"With https, do not verify the certificate of the server.");
    cmd->option(u"output", 'o', Args::FILENAME);
    cmd->help(u"output", u"Save response in the specified file.");
}

ts::URLCommands::~URLCommands()
{
}


//----------------------------------------------------------------------------
// Get URL and dump its text response.
//----------------------------------------------------------------------------

ts::CommandStatus ts::URLCommands::geturl(const UString& command, Args& args)
{
    const bool insecure = args.present(u"insecure");
    const UString url(args.value(u""));
    fs::path output;
    UStringList headers;
    args.getPathValue(output, u"output");
    args.getValues(headers, u"header");

    WebRequest request(args);
    request.setInsecure(insecure);
    for (const auto& h : headers) {
        const size_t colon = h.find(':');
        request.setRequestHeader(h.substr(0, colon).toTrimmed(), colon == NPOS ? u"" : h.substr(colon+1).toTrimmed());
    }

    UString response;
    if (output.empty()) {
        // Display text response.
        if (!request.downloadTextContent(url, response)) {
            return CommandStatus::ERROR;
        }
    }
    else {
        // Save output in a file.
        if (!request.downloadFile(url, output)) {
            return CommandStatus::ERROR;
        }
    }

    args.info(u"==== Request");
    args.info(u"HTTP status: %d", request.httpStatus());
    args.info(u"Original URL: %d", request.originalURL());
    args.info(u"Final URL: %d", request.finalURL());
    args.info(u"==== Response headers");
    for (const auto& h : request.responseHeaders()) {
        args.info(u"%s: %s", h.first, h.second);
    }
    if (output.empty()) {
        args.info(u"==== Response content");
        response.trim(false, true);
        args.info(response);
    }
    return CommandStatus::SUCCESS;
}


//----------------------------------------------------------------------------
// HTTP REST server commands.
//----------------------------------------------------------------------------

// Possible client command to test:
// curl -sikSL 'https://localhost:12345/path/to/api?p1=ab&p2=cd&p3=ef'
//      -H 'Authorization: token boobar'
//      -d 'this is post data' -H 'Content-Type: text/plain'

namespace ts {
    class RESTServerCommands: public CommandLineHandler, protected NetworkBase
    {
        TS_NOBUILD_NOCOPY(RESTServerCommands);
    public:
        RESTServerCommands(CommandLine& cmdline, int flags);
        virtual ~RESTServerCommands() override;

    private:
        RestArgs _rest_args {};
        // Command handlers.
        CommandStatus restServer(const UString&, Args&);
    };
}


//----------------------------------------------------------------------------
// HTTP REST server commands constructor and destructor.
//----------------------------------------------------------------------------

ts::RESTServerCommands::RESTServerCommands(CommandLine& cmdline, int flags)
{
    Args* cmd = cmdline.command(u"rest", u"Basic HTTP REST server which dumps its requests", u"[options] [ip-address:]port", flags);
    cmdline.setCommandLineHandler(this, &RESTServerCommands::restServer, u"rest");
    defineIPGenArgs(*cmd);
    cmd->option(u"", 0, Args::IPSOCKADDR_OA, 1, 1);
    cmd->help(u"", u"TCP server local address.");
    cmd->option(u"max-clients", 'm', Args::UNSIGNED);
    cmd->help(u"max-clients", u"Exit after this number of client sessions. By default, never exit.");
    _rest_args.defineServerArgs(*cmd);
}

ts::RESTServerCommands::~RESTServerCommands()
{
}


//----------------------------------------------------------------------------
// Basic HTTP server which dumps its requests.
//----------------------------------------------------------------------------

ts::CommandStatus ts::RESTServerCommands::restServer(const UString& command, Args& args)
{
    loadIPGenArgs(args);
    _rest_args.loadServerArgs(args, u"");
    size_t max_clients = args.intValue<size_t>(u"max-clients", std::numeric_limits<size_t>::max());

    TCPServer tcp_server;
    TLSServer tls_server(_rest_args);
    TCPServer* const server = _rest_args.use_tls ? &tls_server : &tcp_server;

    if (!server->open(ip_gen, args) ||
        !server->reusePort(true, args) ||
        !server->bind(_rest_args.server_addr, args) ||
        !server->listen(16, args))
    {
        return CommandStatus::ERROR;
    }

    while (max_clients-- > 0) {
        args.verbose(u"Waiting on TCP server %s ...", _rest_args.server_addr);

        TCPConnection tcp_client;
        TLSConnection tls_client;
        tls_client.setVerifyPeer(false);
        TCPConnection* const client = _rest_args.use_tls ? &tls_client : &tcp_client;

        IPSocketAddress addr;
        if (!server->accept(*client, addr, args)) {
            // Failed to accept a client, try a new one.
            continue;
        }
        args.verbose(u"Client connected from %s ...", addr);

        // Process a request.
        RestServer rest(_rest_args, args);
        if (!rest.getRequest(*client)) {
            continue; // to next client
        }

        // Request information.
        args.info(u"==== Request:");
        args.info(u"Method: %s, path: %s", rest.method(), rest.path());
        args.info(u"Query parameters: %d", rest.parameters().size());
        for (const auto& it : rest.parameters()) {
            args.info(u" '%s' -> '%s'", it.first, it.second);
        }
        args.info(u"Request headers: %d", rest.headers().size());
        for (const auto& it : rest.headers()) {
            args.info(u" '%s' -> '%s'", it.first, it.second);
        }
        args.info(u"Token: '%s'", rest.token());
        args.info(u"POST data: %d bytes, type '%s'", rest.postData().size(), rest.postContentType());
        if (!rest.postData().empty()) {
            args.info(u"==== POST data content:");
            if (rest.postContentType().contains(u"text/", CASE_INSENSITIVE)) {
                UString data;
                rest.getPostText(data);
                args.info(data);
            }
            else if (rest.postContentType().contains(u"application/json", CASE_INSENSITIVE)) {
                json::ValuePtr value;
                if (rest.getPostJSON(value) && value != nullptr) {
                    args.info(value->printed(2, args));
                }
            }
            else {
                args.info(u"  %s", UString::Dump(rest.postData(), UString::BPL | UString::HEXA | UString::ASCII, 2, 16).toTrimmed());
            }
            args.info(u"==== End of POST data");
        }             

        // Send some funky response.
        rest.addResponseHeader(u"X-Foo", u"Bar");
        rest.addResponseHeader(u"X-Foo", u"Bar again");
        rest.setResponse(u"This is my response, whether you like it or not.\r\n");
        rest.sendResponse(*client, 200, true);
    }

    server->close(args);
    return CommandStatus::SUCCESS;
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
        ErrorCommands        err {cmdline, flags};
        ZlibCommands         zlib {cmdline, flags};
        NetworkCommands      net {cmdline, flags};
        SendRecvCommands     sendrecv {cmdline, flags};
        ServerCommands       server {cmdline, flags};
        ClientCommands       client {cmdline, flags};
        URLCommands          url {cmdline, flags};
        RESTServerCommands   rest {cmdline, flags};
#if defined(TS_WINDOWS)
        WinModuleCommands    win_module {cmdline, flags};
#endif

        // Inherited methods.
        virtual UString getHelpText(HelpFormat format, size_t line_width = DEFAULT_LINE_WIDTH) const override;
    };
}

// Constructor: get command line options.
ts::DebugCommandOptions::DebugCommandOptions(int argc, char *argv[]) :
    Args(u"TSDuck troubleshooting utility", u"[options] [command args ...]", GATHER_PARAMETERS)
{
    setIntro(u"This application is not offically part of the suite of TSDuck commands. "
             u"It is shipped with TSDuck for troubleshooting issues and as support for the test suite.");

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
