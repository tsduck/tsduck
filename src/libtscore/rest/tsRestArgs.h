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
#include "tsTLSArgs.h"

namespace ts {
    //!
    //! Common arguments for REST API usage.
    //! Can be set by fields or using command line options.
    //! @ingroup libtscore app
    //!
    class TSCOREDLL RestArgs: public TLSArgs
    {
    public:
        //!
        //! Explicit definition of the superclass.
        //!
        using SuperClass = TLSArgs;

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
        UString auth_token {};  //!< Authentication token.
        UString api_root {};    //!< Optional root path for api (e.g. "/serve/api").

        // Server-specific options.
        // No additional option from TLSArgs.

        // Client-specific options.
        cn::milliseconds connection_timeout {};  //!< Connection timeout in milliseconds (zero means none).
        cn::milliseconds receive_timeout {};     //!< Reception timeout in milliseconds (zero means none).

        //!
        //! Add command line options for a REST server in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineServerArgs(Args& args);

        //!
        //! Add some command line options for a REST client in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineClientArgs(Args& args);

        //!
        //! Load arguments for a REST server from a command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] args Command line arguments.
        //! @param [in] server_option Optional name of an option which defines the server port and optional address.
        //! @return True on success, false on error in argument line.
        //!
        bool loadServerArgs(Args& args, const UChar* server_option = nullptr);

        //!
        //! Load arguments for a REST client from a command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] args Command line arguments.
        //! @param [in] server_option Optional name of an option which defines the server name and address.
        //! Resolve server_addr and server_name.
        //! @return True on success, false on error in argument line.
        //!
        bool loadClientArgs(Args& args, const UChar* server_option = nullptr);

    protected:
        UString _opt_token;  //!< Option name for --[prefix-]token.
    };
}
