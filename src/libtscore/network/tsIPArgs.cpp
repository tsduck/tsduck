//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsIPArgs.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::IPArgs::IPArgs(const UString& description, const UString& prefix) :
    _description(description),
    _prefix(prefix + (prefix.empty() || prefix.ends_with('-') ? u"" : u"-"))
{
}

ts::IPArgs::~IPArgs()
{
}


//----------------------------------------------------------------------------
// Command line options for a TLS server.
//----------------------------------------------------------------------------

void ts::IPArgs::defineServerArgs(Args& args)
{
    // No standard command line options are defined at this level.
}


//----------------------------------------------------------------------------
// Load command line options for a TLS server.
//----------------------------------------------------------------------------

bool ts::IPArgs::loadServerArgs(Args& args, const UChar* server_option)
{
    bool status = loadServerAddress(args, server_option);

    // On a server, the port is mandatory, the address is optional
    // (used if the server is bound to a local interface).
    if (status && !server_name.empty() && !server_addr.hasPort()) {
        args.error(u"missing server port in --%s", server_option);
        status = false;
    }

    return status;
}


//----------------------------------------------------------------------------
// Command line options for a TLS client.
//----------------------------------------------------------------------------

void ts::IPArgs::defineClientArgs(Args& args)
{
    // No standard command line options are defined at this level.
}


//----------------------------------------------------------------------------
// Load command line options for a TLS client.
//----------------------------------------------------------------------------

bool ts::IPArgs::loadClientArgs(Args& args, const UChar* server_option)
{
    bool status = loadServerAddress(args, server_option);

    // On a client, server address and port are mandatory. However, this check is
    // already done upstream when the option has been declared with type IPSOCKADDR
    // for instance. If the option was declared as IPSOCKADDR_OA (optional address),
    // omitting the address is explicitly allowed by the application and we must not
    // be more restrictive here. However, for the client to connect to a server, we
    // need an address. In that case, the only sensible default is localhost.
    if (status && !server_name.empty()) {
        if (!server_addr.hasAddress()) {
            server_addr.setAddress(IPAddress::LocalHost4);
            server_name = server_addr.toString();
        }
        if (!server_addr.hasPort()) {
            args.error(u"missing server address or port in --%s", server_option);
            status = false;
        }
    }

    return status;
}


//----------------------------------------------------------------------------
// Get and resolve server name and address.
//----------------------------------------------------------------------------

bool ts::IPArgs::loadServerAddress(Args& args, const UChar* server_option)
{
    bool status = true;
    if (server_option != nullptr) {
        args.getValue(server_name, server_option);
        if (server_name.empty()) {
            // No server name => no server address.
            server_addr.clear();
        }
        else {
            // Resolve address and port.
            status = server_addr.resolve(server_name, args);
        }
    }
    return status;
}


//----------------------------------------------------------------------------
// Common code for loadAllowedClients() and loadDeniedClients().
//----------------------------------------------------------------------------

bool ts::IPArgs::loadAddressesArgs(IPAddressSet IPArgs::* field, Args& args, const UChar* option)
{
    bool success = true;
    const size_t count = args.count(option);
    (this->*field).clear();
    for (size_t i = 0; i < count; ++i) {
        IPAddress addr;
        if (!addr.resolve(args.value(option, u"", i), args)) {
            success = false;
        }
        else if (addr.hasAddress()) {
            (this->*field).insert(addr);
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// On the server side, check if a client address is allowed.
//----------------------------------------------------------------------------

bool ts::IPArgs::isAllowed(const IPAddress& client) const
{
    return (allowed_clients.empty() || allowed_clients.contains(client)) &&
           (denied_clients.empty() || !denied_clients.contains(client));
}
