//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
        //! @param [in] tls_default If false (the default) TLS is not the default and the option <code>--tls</code>
        //! is defined to activate it. If true, TLS is the default and the option <code>--no-tls</code> is defined
        //! to deactivate it and use clear communication.
        //!
        TLSArgs(const UString& description = u"server", const UString& prefix = UString(), bool tls_default = false);

        // Common client and server options.
        bool use_tls = false;            //!< Use SSL/TLS.

        // Server-specific options.
        UString certificate_store {};    //!< TLS server certificate store. @see TLSServerBase::setCertificateStore()
        UString certificate_path {};     //!< TLS server certificate path. @see TLSServerBase::setCertificatePath()
        UString key_path {};             //!< TLS server private key path. @see TLSServerBase::setKeyPath()
        size_t  ephemeral_rsa_bits = 0;  //!< TLS server ephemeral certificate RSA kiey size in bits, when not zero.

        // Client-specific options.
        bool insecure = false;           //!< Do not verify TLS server's certificate.

        //!
        //! Check if enough parameters are provided to specify a certificate, depending on the operating system.
        //! @return True if enough parameters are provided to specify a certificate.
        //!
        bool hasCertificate() const;

        //!
        //! Get the help string which defines the condition of using SSL/TLS.
        //! The actual string depends if SSL/TLS is enabled by default or not.
        //! @return A constant reference to the string "Without --[prefix-]no-tls" or "With --[prefix-]tls".
        //!
        const UString& withTLS() const { return _with_tls; }

        // Inherited methods.
        virtual void defineServerArgs(Args& args) override;
        virtual void defineClientArgs(Args& args) override;
        virtual bool loadServerArgs(Args& args, const UChar* server_option = nullptr) override;
        virtual bool loadClientArgs(Args& args, const UChar* server_option = nullptr) override;

    private:
        bool    _tls_default;            // Use TLS by default.
        UString _opt_tls;                // Option name for --[prefix-]tls.
        UString _opt_no_tls;             // Option name for --[prefix-]no-tls.
        UString _with_tls;               // Help string "Without --[prefix-]no-tls" or "With --[prefix-]tls".
        UString _opt_insecure;           // Option name for --[prefix-]insecure.
        UString _opt_certificate_store;  // Option name for --[prefix-]store.
        UString _opt_certificate_path;   // Option name for --[prefix-]certificate-path.
        UString _opt_key_path;           // Option name for --[prefix-]key-path.
        UString _opt_ephemeral_rsa_bits; // Option name for --[prefix-]ephemeral-rsa-bits.

        // Solve the --tls / --no-tls argument.
        bool loadArgUseTLS(Args& args);
    };
}
