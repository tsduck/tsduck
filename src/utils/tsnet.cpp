//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// Test utility for networking functions.
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsArgs.h"
#include "tsCerrReport.h"
#include "tsNetworkInterface.h"
#include "tsIPAddress.h"
#include "tsUDPSocket.h"
#include "tsTCPServer.h"
#include "tsTelnetConnection.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace ts {
    class NetOptions: public ts::Args
    {
        TS_NOBUILD_NOCOPY(NetOptions);
    public:
        NetOptions(int argc, char *argv[]);
        virtual ~NetOptions() override;

        bool            local = false;
        bool            no_loopback = false;
        IP              gen = IP::Any;
        UString         send_message {};
        UStringVector   resolve_one {};
        UStringVector   resolve_all {};
        IPSocketAddress udp_send {};
        IPSocketAddress udp_receive {};
        IPSocketAddress tcp_send {};
        IPSocketAddress tcp_receive {};
    };
}

ts::NetOptions::~NetOptions()
{
}

ts::NetOptions::NetOptions(int argc, char *argv[]) :
    Args(u"Test utility for networking functions", u"[options] ['message-string']")
{
    option(u"", 0, STRING);
    help(u"", u"Message to send with --udp-send and --tcp-send.");

    option(u"udp-receive", 'u', IPSOCKADDR_OA);
    help(u"udp-receive", u"Wait for a message on the specified UDP socket and send a response.");

    option(u"udp-send", 's', IPSOCKADDR);
    help(u"udp-send", u"Send the 'message-string' to the specified socket and wait for a response.");

    option(u"tcp-receive", 't', IPSOCKADDR_OA);
    help(u"tcp-receive", u"Create a TCP server, wait for a message and send a response.");

    option(u"tcp-send", 'c', IPSOCKADDR);
    help(u"tcp-send", u"Connect to the specified TCP server, send the 'message-string' and wait for a response.");

    option(u"ipv4", '4');
    help(u"ipv4", u"Use only IPv4 addresses.");

    option(u"ipv6", '6');
    help(u"ipv6", u"Use only IPv6 addresses.");

    option(u"no-loopback", 'n');
    help(u"no-loopback", u"With --local, exclude loopback interfaces.");

    option(u"local", 'l');
    help(u"local", u"List local interfaces.");

    option(u"resolve", 'r', STRING, 0, UNLIMITED_COUNT);
    help(u"resolve", u"name", u"Resolve that name once, as in applications.");

    option(u"all-addresses", 'a', STRING, 0, UNLIMITED_COUNT);
    help(u"all-addresses", u"name", u"Get all addresses for that name, as in nslookup.");

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    if (present(u"ipv4")) {
        gen = IP::v4;
    }
    else if (present(u"ipv6")) {
        gen = IP::v6;
    }
    else {
        gen = IP::Any;
    }
    local = present(u"local");
    no_loopback = present(u"no-loopback");
    getValue(send_message, u"");
    getSocketValue(udp_send, u"udp-send");
    getSocketValue(udp_receive, u"udp-receive");
    getSocketValue(tcp_send, u"tcp-send");
    getSocketValue(tcp_receive, u"tcp-receive");
    getValues(resolve_one, u"resolve");
    getValues(resolve_all, u"all-addresses");

    // Final checking
    exitOnError();
}


//----------------------------------------------------------------------------
// Full image of an IP address.
//----------------------------------------------------------------------------

namespace {
    ts::UString Format(const ts::IPAddress& addr)
    {
        return ts::UString::Format(u"%s: %s (full: \"%s\")", addr.familyName(), addr, addr.toFullString());
    }
}


//----------------------------------------------------------------------------
//  Program main code.
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    // Get command line options.
    ts::NetOptions opt(argc, argv);
    CERR.setMaxSeverity(opt.maxSeverity());

    // Resolve one host name.
    for (const auto& name : opt.resolve_one) {
        ts::IPAddress addr;
        if (addr.resolve(name, opt, opt.gen)) {
            std::cout << "Resolve \"" << name << "\":" << std::endl;
            std::cout << "  " << Format(addr) << std::endl;
        }
    }

    // Resolve all addresses for one host name.
    for (const auto& name : opt.resolve_all) {
        ts::IPAddressVector addr;
        if (ts::IPAddress::ResolveAllAddresses(addr, name, opt, opt.gen)) {
            std::cout << "Resolve \"" << name << "\":" << std::endl;
            for (const auto& a : addr) {
                std::cout << "  " << Format(a) << std::endl;
            }
        }
    }

    // Get local interfaces.
    if (opt.local) {
        ts::NetworkInterfaceVector net;
        if (ts::NetworkInterface::GetAll(net, !opt.no_loopback, opt.gen, false, opt)) {
            std::cout << "Local interfaces: " << net.size() << std::endl;
            for (const auto& n : net) {
                std::cout << "  " << n << std::endl;
            }
        }
    }

    // Receive a UDP message, send a response.
    if (opt.udp_receive.hasPort()) {
        ts::UDPSocket sock;
        if (sock.open(opt.gen, opt)) {
            opt.info(u"Waiting on UDP socket %s ...", opt.udp_receive);
            std::string msg(8192, '\0');
            size_t ret_size = 0;
            ts::IPSocketAddress source, destination;
            if (sock.reusePort(true, opt) &&
                sock.bind(opt.udp_receive, opt) &&
                sock.receive(msg.data(), msg.size(), ret_size, source, destination, nullptr, opt))
            {
                msg.resize(ret_size);
                opt.info(u"Received %d bytes: \"%s\"", ret_size, msg);
                opt.info(u"Source: %s, destination: %s", source, destination);
                msg.insert(0, "-> [");
                msg.append("]");
                sock.send(msg.data(), msg.size(), source, opt);
            }
            sock.close(opt);
        }
    }

    // Send a UDP message, wait for the response.
    if (opt.udp_send.hasAddress()) {
        ts::UDPSocket sock;
        if (sock.open(opt.gen, opt)) {
            opt.info(u"Sending to UDP socket %s ...", opt.udp_send);
            std::string msg(opt.send_message.toUTF8());
            if (sock.bind(ts::IPSocketAddress::AnySocketAddress(opt.gen), opt) &&
                sock.send(msg.data(), msg.size(), opt.udp_send, opt))
            {
                size_t ret_size = 0;
                ts::IPSocketAddress source, destination;
                msg.resize(8192);
                if (sock.receive(msg.data(), msg.size(), ret_size, source, destination, nullptr, opt)) {
                    msg.resize(ret_size);
                    opt.info(u"Received %d bytes: \"%s\"", ret_size, msg);
                    opt.info(u"Source: %s, destination: %s", source, destination);
                }
            }
            sock.close(opt);
        }
    }

    // TCP server, wait for a client, wait for a message, send a response.
    if (opt.tcp_receive.hasPort()) {
        ts::TCPServer server;
        if (server.open(opt.gen, opt)) {
            if (server.reusePort(true, opt) &&
                server.bind(opt.tcp_receive, opt) &&
                server.listen(1, opt))
            {
                opt.info(u"Waiting on TCP server %s ...", opt.tcp_receive);
                ts::TelnetConnection client;
                ts::IPSocketAddress addr;
                if (server.accept(client, addr, opt)) {
                    opt.info(u"Client connected from %s ...", addr);
                    std::string msg;
                    if (client.receiveLine(msg, nullptr, opt)) {
                        opt.info(u"Received line: \"%s\"", msg);
                        msg.insert(0, "-> [");
                        msg.append("]");
                        client.sendLine(msg, opt);
                    }
                    client.close();
                }
            }
            server.close(opt);
        }
    }

    // Send a TCP message, wait for the response.
    if (opt.tcp_send.hasAddress()) {
        ts::TelnetConnection client;
        if (client.open(opt.gen, opt)) {
            opt.info(u"Sending to TCP server %s ...", opt.tcp_send);
            std::string msg(opt.send_message.toUTF8());
            ts::IPSocketAddress addr;
            if (client.bind(ts::IPSocketAddress::AnySocketAddress(opt.gen), opt) &&
                client.connect(opt.tcp_send, opt) &&
                client.getLocalAddress(addr, opt) &&
                client.sendLine(msg, opt) &&
                client.receiveLine(msg, nullptr, opt))
            {
                opt.info(u"Client address: %s", addr);
                opt.info(u"Received line: \"%s\"", msg);
            }
            client.close(opt);
        }
    }

    return EXIT_SUCCESS;
}
