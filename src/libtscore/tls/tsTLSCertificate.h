//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  SSL/TLS server certificate.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReporterBase.h"
#include "tsTLSServerBase.h"

namespace ts {
    //!
    //! Encapsulate a SSL/TLS server certificate.
    //! @ingroup libtscore net
    //!
    class TSCOREDLL TLSCertificate: public ReporterBase
    {
        TS_NOBUILD_NOCOPY(TLSCertificate);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit TLSCertificate(Report* report, Object* owner = nullptr);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit TLSCertificate(ReporterBase* delegate, Object* owner = nullptr);

        //!
        //! Destructor.
        //!
        virtual ~TLSCertificate() override;

        //!
        //! Reset the content of the certificate.
        //!
        void reset();

        //!
        //! Get the certificate context, for usage in OpenSSL (UNIX) or SChannel (Windows).
        //! @return The certificate context, or a null pointer if none is available.
        //! - On UNIX systems with OpenSSL, a pointer to SSL_CTX.
        //! - On Windows systems whith SChannel, a pointer to CERT_CONTEXT.
        //!
        void* getCertificate() const;

        //!
        //! Check if a certificate is loaded and valid.
        //! @return Trues if a certificate is loaded and valid, false otherwise.
        //!
        bool isValid() const { return getCertificate() != nullptr; }

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
        //! @param [in] certificate_path Path to the certificate.
        //! - On UNIX systems (with OpenSSL), this is the path name of the certificate file in PEM format.
        //! - On Windows, this is the name of a certificate, either its "friendly name", its subject name (without "CN="), its DNS name.
        //! @param [in] key_path Path to the private key.
        //! - On UNIX systems (with OpenSSL), this is the path name of the private key file in PEM format.
        //! - On Windows, the private key is retrieved with the certificate and this parameter is unused.
        //! @param [in] store_name Name of certificate store.
        //! - On UNIX systems (with OpenSSL), this parameter is unused.
        //! - On Windows, the possible values are "system" (<code>Cert:\\LocalMachine\\My</code>)
        //!   and "user" (<code>Cert:\\CurrentUser\\My</code>). The default is "user".
        //! @return True on success, false on error.
        //!
        bool loadCertificate(const UString& certificate_path, const UString& key_path, const UString& store_name);

        //!
        //! Initialize (get or create) a server certificate, if not already done.
        //! If a certificate is already present, don't replace it.
        //! @param [in] params Server parameters.
        //! @return True on success, false on error.
        //!
        bool initServerCertificate(const TLSServerBase& params);

    private:
        // System-specific parts are stored in a private structure.
        // This is done to avoid inclusion of specialized headers in this public file.
        class SystemGuts;
        SystemGuts* _guts = nullptr;

        // Allocate and deallocate guts (depend on implementations).
        void allocateGuts();
        void deleteGuts();

        // Get the certificate subject (CN), built from the hostname.
        // Note: CN is now informational only and no longer used in certificate validation by most clients.
        // The CN field is limited to 64 characters and this function returns the 64 right-most characters of the hostname.
        static UString Subject();
    };
}
