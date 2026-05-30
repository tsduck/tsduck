//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  SChannel certificate (Windows-specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReporterBase.h"
#include "tsTLSServerBase.h"
#include "tsWinTLS.h"

namespace ts {
    //!
    //! Encapsulate a SChannel certificate (Windows-specific).
    //! @ingroup libtscore windows
    //!
    class TSCOREDLL SChannelCertificate: public ReporterBase
    {
        TS_NOBUILD_NOCOPY(SChannelCertificate);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //!
        explicit SChannelCertificate(Report* report);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //!
        explicit SChannelCertificate(ReporterBase* delegate);

        //!
        //! Destructor.
        //!
        virtual ~SChannelCertificate() override;

        //!
        //! Reset the content of the certificate.
        //!
        void reset();

        //!
        //! Get the certificate context, for usage in SChannel.
        //! @return The certificate context, or a null pointer if none is available.
        //!
        ::PCCERT_CONTEXT getCertificate() const { return _cert; }

        //!
        //! Check if a certificate is loaded and valid.
        //! @return Trues if a certificate is loaded and valid, false otherwise.
        //!
        bool isValid() const { return _cert != nullptr; }

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
        //! @param [in] store_name Name of certificate store. One of "system", "user".
        //! @param [in] cert_name Name of the certificate (friendly name or subject name or DNS name).
        //! @return True on success, false on error.
        //!
        bool loadCertificate(const UString& store_name, const UString& cert_name);

        //!
        //! Initialize (get or create) a server certificate, if not already done.
        //! If a certificate is already present, don't replace it.
        //! @param [in] params Server parameters.
        //! @return True on success, false on error.
        //!
        bool initServerCertificate(const TLSServerBase& params);

        //!
        //! Get the name of a certificate name for a given type.
        //! @param [in] cert Certificate handle.
        //! @param [in] type Type of name (CERT_NAME_FRIENDLY_DISPLAY_TYPE, CERT_NAME_xxx).
        //! @return Certificate name, empty string on error.
        //!
        static UString GetCertificateName(::PCCERT_CONTEXT cert, ::DWORD type);

    private:
        ::PCCERT_CONTEXT     _cert = nullptr;
        ::NCRYPT_PROV_HANDLE _provider = 0;
        ::NCRYPT_KEY_HANDLE  _key = 0;

        // Repository of Windows certificate stores.
        // The certificate stores must remain open all the time, once open.
        // They are closed on termination of the singleton.
        class TSCOREDLL CertStoreRepository
        {
            TS_SINGLETON(CertStoreRepository);
        public:
            // Destructor: close opened stores.
            ~CertStoreRepository();

            // Get or open a certificate store. Name of certificate store must be "system" or "user".
            ::HCERTSTORE getStore(const UString& store_name, Report& report);

        private:
            std::mutex _mutex {};
            std::map<UString, ::HCERTSTORE> _stores {};
        };
    };
}
