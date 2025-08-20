//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Common arguments for TLS clients and servers.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"

namespace ts {
    //!
    //! Common arguments for TLS client and server usage.
    //! Can be set by fields or using command line options.
    //! @ingroup libtscore app
    //!
    class TSCOREDLL TLSArgs
    {
    public:
        //!
        //! Constructor.
        //! @param [in] description Short description of the TLS service.
        //! Example: <code>"control port"</code>. Use no initial cap, no final dot.
        //! @param [in] prefix Optional prefix for all command line options.
        //! Example: when @a prefix is <code>"foo"</code>, the option <code>--certificate-path</code>
        //! becomes <code>--foo-certificate-path</code>.
        //!
        TLSArgs(const UString& description = u"server", const UString& prefix = UString());

        // Common client and server options.
        bool use_tls = false;             //!< Use SSL/TLS.
        IPSocketAddress server_addr {};   //!< Server address and port. The address is optional on server side.

        // Server-specific options.
        IPAddressSet allowed_clients {};  //!< White-list of allowed incoming clients.
        IPAddressSet denied_clients {};   //!< Black-list of denied incoming clients.
        UString certificate_store {};     //!< TLS server certificate store. @see TLSServer::setCertificateStore()
        UString certificate_path {};      //!< TLS server certificate path. @see TLSServer::setCertificatePath()
        UString key_path {};              //!< TLS server private key path. @see TLSServer::setKeyPath()

        // Client-specific options.
        bool insecure = false;            //!< Do not verify TLS server's certificate.
        UString server_name {};           //!< Server host name (required in addition to server address for TLS).

        //!
        //! Add command line options for a TLS server in an Args.
        //! No options is defined for server [addr:]port because the description is probably too specific.
        //! Same for lists of allowed and denied clients.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineServerArgs(Args& args);

        //!
        //! Add some command line options for a TLS client in an Args.
        //! No options is defined for server addr:port because the description is probably too specific.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineClientArgs(Args& args);

        //!
        //! Load arguments for a TLS server from a command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] args Command line arguments.
        //! @param [in] server_option Optional name of an option which defines the server port and optional address.
        //! @return True on success, false on error in argument line.
        //!
        bool loadServerArgs(Args& args, const UChar* server_option = nullptr);

        //!
        //! Load arguments for a TLS client from a command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] args Command line arguments.
        //! @param [in] server_option Optional name of an option which defines the server name and address.
        //! Resolve server_addr and server_name.
        //! @return True on success, false on error in argument line.
        //!
        bool loadClientArgs(Args& args, const UChar* server_option = nullptr);

        //!
        //! Load the set of allowed clients from a command line (server side).
        //! @param [in,out] args Command line arguments.
        //! @param [in] option Option which defines the client addresses.
        //! @return True on success, false on error in argument line.
        //!
        bool loadAllowedClients(Args& args, const UChar* option) { return loadAddressesArgs(&TLSArgs::allowed_clients, args, option); }

        //!
        //! Load the set of denied clients from a command line (server side).
        //! @param [in,out] args Command line arguments.
        //! @param [in] option Option which defines the client addresses.
        //! @return True on success, false on error in argument line.
        //!
        bool loadDeniedClients(Args& args, const UChar* option) { return loadAddressesArgs(&TLSArgs::denied_clients, args, option); }

        //!
        //! On the server side, check if a client address is allowed, based on sets of allowed and denied clients.
        //! @param [in] client Incoming client address.
        //! @return True if the client is allowed, false if it is denied.
        //! 
        bool isAllowed(const IPAddress& client) const;

    protected:
        UString _description;            //!< Short description of the TLS service.
        UString _prefix;                 //!< Option prefix, ready to use in other option names.
        UString _opt_tls;                //!< Option name for --[prefix-]tls.
        UString _opt_insecure;           //!< Option name for --[prefix-]insecure.
        UString _opt_certificate_store;  //!< Option name for --[prefix-]store.
        UString _opt_certificate_path;   //!< Option name for --[prefix-]certificate-path.
        UString _opt_key_path;           //!< Option name for --[prefix-]key-path.

    private:
        // Common code for loadAllowedClients() and loadDeniedClients().
        bool loadAddressesArgs(IPAddressSet TLSArgs::*, Args&, const UChar* option);
    };
}
