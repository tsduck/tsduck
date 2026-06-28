//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Common properties for all forms of SSL/TLS Server
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTLSArgs.h"

namespace ts {
    //!
    //! Common properties for all forms of SSL/TLS server.
    //! @ingroup libtscore net
    //!
    class TSCOREDLL TLSServerBase
    {
        TS_DEFAULT_COPY_MOVE(TLSServerBase);
    public:
        //!
        //! Set command line arguments for the server.
        //! @param [in] args TLS arguments.
        //!
        void setArgs(const TLSArgs& args) { _tls_args = args; }

        //!
        //! Set the certificate path for the server.
        //! @param [in] path Path to the certificate.
        //! - On UNIX systems (with OpenSSL), this is the path name of the certificate file in PEM format.
        //! - On Windows, this is the name of a certificate, either its "friendly name", its subject name (without "CN="), its DNS name.
        //!
        void setCertificatePath(const UString& path) { _tls_args.certificate_path = path; }

        //!
        //! Get the certificate path for the server.
        //! @return A constant reference to the path to the certificate.
        //! @see setCertificatePath()
        //!
        const UString& getCertificatePath() const { return _tls_args.certificate_path; }

        //!
        //! Set the private key path for the server.
        //! @param [in] path Path to the private key.
        //! - On UNIX systems (with OpenSSL), this is the path name of the private key file in PEM format.
        //! - On Windows, the private key is retrieved with the certificate and this parameter is unused.
        //!
        void setKeyPath(const UString& path) { _tls_args.key_path = path; }

        //!
        //! Get the private key path for the server.
        //! @return A constant reference to the path to the private key.
        //! @see setKeyPath()
        //!
        const UString& getKeyPath() const { return _tls_args.key_path; }

        //!
        //! Set the certificate store.
        //! @param [in] name
        //! - On UNIX systems (with OpenSSL), this parameter is unused.
        //! - On Windows, the possible values are "system" (<code>Cert:\\LocalMachine\\My</code>)
        //!   and "user" (<code>Cert:\\CurrentUser\\My</code>). The default is "user".
        //!
        void setCertificateStore(const UString& name) { _tls_args.certificate_store = name; }

        //!
        //! Get the certificate store.
        //! @return A constant reference to the name of the certificate store.
        //! @see setCertificateStore()
        //!
        const UString& getCertificateStore() const { return _tls_args.certificate_store; }

        //!
        //! Specify to use an ephemeral self-signed certificate with an ephemeral RSA key of the specified size.
        //! @param [in] bits Size in bits of the ephemeral RSA key. When set to zero, no ephemeral self-signed
        //! certificate is used and a persistent certificate must be used.
        //!
        void setEphemeralRSABits(size_t bits) { _tls_args.ephemeral_rsa_bits = bits; }

        //!
        //! Get the size in bits of the ephemeral RSA key which is used for the ephemeral self-signed certificate.
        //! @return Size in bits of the ephemeral RSA key. When zero, no ephemeral self-signed certificate is used.
        //!
        size_t getEphemeralRSABits() const { return _tls_args.ephemeral_rsa_bits; }

        //!
        //! Virtual destructor.
        //!
        virtual ~TLSServerBase();

    protected:
        //!
        //! Constructor is accessible to subclasses only.
        //!
        TLSServerBase() = default;
        
    private:
        TLSArgs _tls_args {};
    };
}
