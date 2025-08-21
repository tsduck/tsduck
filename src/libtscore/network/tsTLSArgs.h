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
#include "tsIPArgs.h"

namespace ts {
    //!
    //! Common arguments for TLS client and server usage.
    //! Can be set by fields or using command line options.
    //! @ingroup libtscore app
    //!
    class TSCOREDLL TLSArgs: public IPArgs
    {
        TS_RULE_OF_FIVE(TLSArgs, override);
    public:
        //!
        //! Explicit definition of the superclass.
        //!
        using SuperClass = IPArgs;

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
        bool use_tls = false;          //!< Use SSL/TLS.

        // Server-specific options.
        UString certificate_store {};  //!< TLS server certificate store. @see TLSServer::setCertificateStore()
        UString certificate_path {};   //!< TLS server certificate path. @see TLSServer::setCertificatePath()
        UString key_path {};           //!< TLS server private key path. @see TLSServer::setKeyPath()

        // Client-specific options.
        bool insecure = false;         //!< Do not verify TLS server's certificate.

        // Inherited methods.
        virtual void defineServerArgs(Args& args) override;
        virtual void defineClientArgs(Args& args) override;
        virtual bool loadServerArgs(Args& args, const UChar* server_option = nullptr) override;
        virtual bool loadClientArgs(Args& args, const UChar* server_option = nullptr) override;

    protected:
        UString _opt_tls;                //!< Option name for --[prefix-]tls.
        UString _opt_insecure;           //!< Option name for --[prefix-]insecure.
        UString _opt_certificate_store;  //!< Option name for --[prefix-]store.
        UString _opt_certificate_path;   //!< Option name for --[prefix-]certificate-path.
        UString _opt_key_path;           //!< Option name for --[prefix-]key-path.
    };
}
