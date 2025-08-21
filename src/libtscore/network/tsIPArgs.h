//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Common arguments for IP clients and servers, UDP and TCP.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"

namespace ts {
    //!
    //! Common arguments for IP clients and servers, UDP and TCP.
    //! Can be set by fields or using command line options.
    //! @ingroup libtscore app
    //!
    class TSCOREDLL IPArgs
    {
        TS_RULE_OF_FIVE(IPArgs,);
    public:
        //!
        //! Constructor.
        //! @param [in] description Short description of the TLS service.
        //! Example: <code>"control port"</code>. Use no initial cap, no final dot.
        //! @param [in] prefix Optional prefix for all command line options.
        //! Example: when @a prefix is <code>"foo"</code>, the option <code>--certificate-path</code>
        //! becomes <code>--foo-certificate-path</code>.
        //!
        IPArgs(const UString& description = u"server", const UString& prefix = UString());

        // Common client and server options.
        IPSocketAddress  server_addr {};         //!< Server address and port. The address is optional on server side.
        UString          server_name {};         //!< Server host name (required in addition to server address in some cases).
        cn::milliseconds receive_timeout {0};    //!< Reception timeout in milliseconds (zero means none).
        bool             reuse_port = true;      //!< Reuse-port socket option.

        // Server-specific options.
        IPAddressSet     allowed_clients {};     //!< White-list of allowed incoming clients.
        IPAddressSet     denied_clients {};      //!< Black-list of denied incoming clients.

        // Client-specific options.
        cn::milliseconds connection_timeout {0}; //!< Connection timeout in milliseconds (zero means none).

        //!
        //! Add command line options for a server in an Args.
        //! No options is defined for server [addr:]port because the description is probably too specific.
        //! Same for lists of allowed and denied clients.
        //! @param [in,out] args Command line arguments to update.
        //!
        virtual void defineServerArgs(Args& args);

        //!
        //! Add some command line options for a client in an Args.
        //! No options is defined for server addr:port because the description is probably too specific.
        //! @param [in,out] args Command line arguments to update.
        //!
        virtual void defineClientArgs(Args& args);

        //!
        //! Load arguments for a server from a command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] args Command line arguments.
        //! @param [in] server_option Optional name of an option which defines the server port and optional address.
        //! @return True on success, false on error in argument line.
        //!
        virtual bool loadServerArgs(Args& args, const UChar* server_option = nullptr);

        //!
        //! Load arguments for a client from a command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] args Command line arguments.
        //! @param [in] server_option Optional name of an option which defines the server name and address.
        //! @return True on success, false on error in argument line.
        //!
        virtual bool loadClientArgs(Args& args, const UChar* server_option = nullptr);

        //!
        //! Load the set of allowed clients from a command line (server side).
        //! @param [in,out] args Command line arguments.
        //! @param [in] option Option which defines the client addresses.
        //! @return True on success, false on error in argument line.
        //!
        bool loadAllowedClients(Args& args, const UChar* option) { return loadAddressesArgs(&IPArgs::allowed_clients, args, option); }

        //!
        //! Load the set of denied clients from a command line (server side).
        //! @param [in,out] args Command line arguments.
        //! @param [in] option Option which defines the client addresses.
        //! @return True on success, false on error in argument line.
        //!
        bool loadDeniedClients(Args& args, const UChar* option) { return loadAddressesArgs(&IPArgs::denied_clients, args, option); }

        //!
        //! On the server side, check if a client address is allowed, based on sets of allowed and denied clients.
        //! @param [in] client Incoming client address.
        //! @return True if the client is allowed, false if it is denied.
        //! 
        bool isAllowed(const IPAddress& client) const;

    protected:
        UString _description;  //!< Short description of the TLS service.
        UString _prefix;       //!< Option prefix, ready to use in other option names.

    private:
        // Get and resolve server name and address.
        bool loadServerAddress(Args& args, const UChar* server_option);

        // Common code for loadAllowedClients() and loadDeniedClients().
        bool loadAddressesArgs(IPAddressSet IPArgs::*, Args&, const UChar* option);
    };
}
