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
#include "tsIPUtils.h"
#include "tsIPAddress.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace {
    class NetOptions: public ts::Args
    {
        TS_NOBUILD_NOCOPY(NetOptions);
    public:
        NetOptions(int argc, char *argv[]);

        bool              local = false;
        bool              no_loopback = false;
        ts::IP            gen = ts::IP::Any;
        ts::UStringVector resolve_one {};
        ts::UStringVector resolve_all {};
    };
}

NetOptions::NetOptions(int argc, char *argv[]) :
    Args(u"Test utility for networking functions", u"[options]")
{
    option(u"ipv4", '4');
    help(u"ipv4", u"Use only IPv4 addresses.");

    option(u"ipv6", '6');
    help(u"ipv6", u"Use only IPv6 addresses.");

    option(u"local", 'l');
    help(u"local", u"List local interfaces.");

    option(u"no-loopback", 'n');
    help(u"no-loopback", u"With --local, exclude loopback interfaces.");

    option(u"resolve", 'r', STRING, 0, UNLIMITED_COUNT);
    help(u"resolve", u"name", u"Resolve that name once, as in applications.");

    option(u"all-addresses", 'a', STRING, 0, UNLIMITED_COUNT);
    help(u"all-addresses", u"name", u"Get all addresses for that name, as in nslookup.");

    // Analyze the command.
    analyze(argc, argv);

    // Load option values.
    local = present(u"local");
    no_loopback = present(u"no-loopback");
    if (present(u"ipv4")) {
        gen = ts::IP::v4;
    }
    else if (present(u"ipv6")) {
        gen = ts::IP::v6;
    }
    else {
        gen = ts::IP::Any;
    }
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
    NetOptions opt(argc, argv);
    CERR.setMaxSeverity(opt.maxSeverity());

    for (const auto& name : opt.resolve_one) {
        ts::IPAddress addr(opt.gen);
        if (addr.resolve(name, CERR)) {
            std::cout << "Resolve \"" << name << "\":" << std::endl;
            std::cout << "  " << Format(addr) << std::endl;
        }
    }

    for (const auto& name : opt.resolve_all) {
        ts::IPAddressVector addr;
        if (ts::IPAddress::ResolveAllAddresses(addr, name, CERR, opt.gen)) {
            std::cout << "Resolve \"" << name << "\":" << std::endl;
            for (const auto& a : addr) {
                std::cout << "  " << Format(a) << std::endl;
            }
        }
    }

    if (opt.local) {
        ts::IPAddressMaskVector addr;
        if (ts::GetLocalIPAddresses(addr, !opt.no_loopback, opt.gen, CERR)) {
            std::cout << "Local interfaces: " << addr.size() << std::endl;
            for (const auto& a : addr) {
                std::cout << "  " << Format(a) << std::endl;
            }
        }
    }

    return EXIT_SUCCESS;
}
