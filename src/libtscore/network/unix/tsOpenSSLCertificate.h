//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  OpenSSL certificate (UNIX-specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReporterBase.h"
#include "tsTLSServerBase.h"
#include "tsOpenSSL.h"

namespace ts {
    //!
    //! Encapsulate an OpenSSL certificate (UNIX-specific).
    //! @ingroup libtscore unix
    //!
    class TSCOREDLL OpenSSLCertificate: public ReporterBase, public OpenSSL::Controlled
    {
        TS_NOBUILD_NOCOPY(OpenSSLCertificate);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //!
        explicit OpenSSLCertificate(Report* report);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //!
        explicit OpenSSLCertificate(ReporterBase* delegate);

        //!
        //! Destructor.
        //!
        virtual ~OpenSSLCertificate() override;

        //!
        //! Get the certificate context, for usage in OpenSSL.
        //! @return The certificate context, or a null pointer if none is available.
        //!
        SSL_CTX* getCertificate() const { return _ssl_ctx; }

        //!
        //! Check if a certificate is loaded and valid.
        //! @return Trues if a certificate is loaded and valid, false otherwise.
        //!
        bool isValid() const { return _ssl_ctx != nullptr; }

        //!
        //! Create an ephemeral self-signed certificate.
        //! The previous certificate, if any, is replaced.
        //! @param [in] rsa_bits Size in bits of the RSA key to create for the certificate.
        //! @return True on success, false on error.
        //!
        bool createEphemeralCertificate(size_t rsa_bits);

        //!
        //! Load a certificate from a store.
        //! The previous certificate, if any, is replaced.
        //! @param [in] certificate_path Path name of the certificate file in PEM format.
        //! @param [in] key_path Path name of the private key file in PEM format.
        //! @return True on success, false on error.
        //!
        bool loadCertificate(const UString& certificate_path, const UString& key_path);

        //!
        //! Initialize (get or create) a server certificate, if not already done.
        //! If a certificate is already present, don't replace it.
        //! @param [in] params Server parameters.
        //! @return True on success, false on error.
        //!
        bool initServerCertificate(const TLSServerBase& params);

        // Implementation of OpenSSL::Controlled.
        virtual void terminate() override;

    private:
        SSL_CTX* _ssl_ctx = nullptr;

        // Add to certificate 'cert' X.509v3 extension id 'nid'
        bool addExtension(X509* cert, int nid, const char* value);
    };
}
