//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Common arguments for REST API usage.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"

namespace ts {
    //!
    //! Common arguments for REST API usage.
    //! Can be set by fields or using command line options.
    //! @ingroup libtscore app
    //!
    class TSCOREDLL RestArgs
    {
    public:
        //!
        //! Constructor.
        //! @param [in] description Short description of the REST service.
        //! Example: <code>"control port"</code>. Use no initial cap, no final dot.
        //! @param [in] prefix Optional prefix for all command line options.
        //! Example: when @a prefix is <code>"foo"</code>, the option <code>--certificate-path</code>
        //! becomes <code>--foo-certificate-path</code>.
        //!
        RestArgs(const UString& description = u"server", const UString& prefix = UString());

        // Common client and server options.
        bool    use_tls = false;  //!< Use SSL/TLS.
        UString auth_token {};    //!< Authentication token.
        UString api_root {};      //!< Optional root path for api (e.g. "/serve/api").

        // Server-specific options.
        UString certificate_store {};  //!< TLS server certificate store. @see TLSServer::setCertificateStore()
        UString certificate_path {};   //!< TLS server certificate path. @see TLSServer::setCertificatePath()
        UString key_path {};           //!< TLS server private key path. @see TLSServer::setKeyPath()

        // Client-specific options.
        cn::milliseconds connection_timeout {};  //!< Connection timeout in milliseconds (zero means none).
        cn::milliseconds receive_timeout {};     //!< Reception timeout in milliseconds (zero means none).
        bool             insecure = false;       //!< Do not verify TLS server's certificate.
        UString          server_name {};         //!< Server host name. Not in command line arguments.
        uint16_t         server_port = 0;        //!< Server port (0 means default for HTTP or HTTPS).

        //!
        //! Add command line options for a REST server in an Args.
        //! @param [in,out] args Command line arguments to update.
        //! @param [in] with_auth Include authentication options.
        //!
        void defineServerArgs(Args& args, bool with_auth = true);

        //!
        //! Add some command line options for a REST client in an Args.
        //! @param [in,out] args Command line arguments to update.
        //! @param [in] with_auth Include authentication options.
        //!
        void defineClientArgs(Args& args, bool with_auth = true);

        //!
        //! Load arguments for a REST server from a command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] args Command line arguments.
        //! @param [in] with_auth Include authentication options.
        //! @return True on success, false on error in argument line.
        //!
        bool loadServerArgs(Args& args, bool with_auth = true);

        //!
        //! Load arguments for a REST client from a command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] args Command line arguments.
        //! @param [in] with_auth Include authentication options.
        //! @return True on success, false on error in argument line.
        //!
        bool loadClientArgs(Args& args, bool with_auth = true);

    private:
        UString _description;
        UString _prefix;
        UString _opt_tls;
        UString _opt_insecure;
        UString _opt_token;
        UString _opt_certificate_store;
        UString _opt_certificate_path;
        UString _opt_key_path;
    };
}
