//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsWinTLS.h"


//----------------------------------------------------------------------------
// Get a certificate.
//----------------------------------------------------------------------------

::PCCERT_CONTEXT ts::GetCertificate(const UString& store_name, const UString& cert_name, Report& report)
{
    // We need a certificate name.
    if (cert_name.empty()) {
        report.error(u"no TLS certificate is specified");
        return nullptr;
    }

    // Get the certificate store.
    ::HCERTSTORE store = CertStoreRepository::Instance().getStore(store_name, report);
    if (store == nullptr) {
        return nullptr;
    }

    // Search the certificate in the store. Only consider certificates with a private key.
    ::PCCERT_CONTEXT cert = nullptr;
    while ((cert = ::CertFindCertificateInStore(store, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_HAS_PRIVATE_KEY, nullptr, cert)) != nullptr) {
        if (GetCertificateName(cert, CERT_NAME_FRIENDLY_DISPLAY_TYPE) == cert_name ||
            GetCertificateName(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE) == cert_name ||
            GetCertificateName(cert, CERT_NAME_DNS_TYPE) == cert_name)
        {
            report.debug(u"found certificate \"%s\"", cert_name);
            return cert;
        }
    }

    // Certificate not found: not found or error?
    const ::DWORD err = ::GetLastError();
    if (err == CRYPT_E_NOT_FOUND) {
        report.error(u"certificate \"%s\" not found", cert_name);
    }
    else {
        report.error(u"error searching certificate \"%s\": %s", cert_name, WinErrorMessage(err));
    }
    return nullptr;
}


//----------------------------------------------------------------------------
// Get a certificate name.
//----------------------------------------------------------------------------

ts::UString ts::GetCertificateName(::PCCERT_CONTEXT cert, ::DWORD type)
{
    ::DWORD size = ::CertGetNameStringW(cert, type, 0, nullptr, nullptr, 0);
    std::vector<::WCHAR> name(std::max<size_t>(1, size));
    size = ::CertGetNameStringW(cert, type, 0, nullptr, name.data(), ::DWORD(name.size()));
    return UString(name.data(), std::max<size_t>(1, std::min<size_t>(name.size(), size)) - 1);
}


//----------------------------------------------------------------------------
// Certificate stores.
//----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::CertStoreRepository);
ts::CertStoreRepository::CertStoreRepository() {}

// Destructor: close opened stores.
ts::CertStoreRepository::~CertStoreRepository()
{
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& it : _stores) {
        if (it.second != nullptr) {
            ::CertCloseStore(it.second, CERT_CLOSE_STORE_FORCE_FLAG);
        }
    }
    _stores.clear();
}

// Get or open a certificate store.
::HCERTSTORE ts::CertStoreRepository::getStore(const UString& store_name, Report& report)
{
    // Resolve certificate store name.
    ::DWORD flags = CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG;
    if (store_name == u"user") {
        flags |= CERT_SYSTEM_STORE_CURRENT_USER;
    }
    else if (store_name == u"system") {
        flags |= CERT_SYSTEM_STORE_LOCAL_MACHINE;
    }
    else {
        report.error(u"invalid certificate store name \"%s\"", store_name);
        return nullptr;
    }

    // Get the store in the map, under lock protection.
    std::lock_guard<std::mutex> lock(_mutex);
    if (_stores.contains(store_name)) {
        return _stores[store_name];
    }
    else {
        report.debug(u"opening certificate store \"%s\"", store_name);
        ::HCERTSTORE hcs = ::CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, flags, L"My");
        if (hcs != nullptr) {
            return _stores[store_name] = hcs;
        }
        else {
            report.error(u"error opening certificate store \"%s\": %s", store_name, WinErrorMessage(::GetLastError()));
            return nullptr;
        }
    }
}


//----------------------------------------------------------------------------
// Acquire TLS credentials.
//----------------------------------------------------------------------------

bool ts::GetCredentials(::CredHandle& cred, bool server, bool verify_peer, ::PCCERT_CONTEXT cert, Report& report)
{
    // AcquireCredentialsHandle needs a non-const string (although it does not modify it).
    static UString unisp_name(UNISP_NAME_W);

    // TLS parameters: disallow everything that is not TLS 1.2, 1.3 or higher.
    ::TLS_PARAMETERS tls_params {
        //@@@ .grbitDisabledProtocols = ::DWORD(~(SP_PROT_TLS1_2 | SP_PROT_TLS1_3PLUS)),
        .grbitDisabledProtocols = ::DWORD(~(SP_PROT_TLS1_2)),
    };

    const ::ULONG use = server ? SECPKG_CRED_INBOUND : SECPKG_CRED_OUTBOUND;
    ::SCH_CREDENTIALS credentials {
        .dwVersion = SCH_CREDENTIALS_VERSION,
        .cCreds = ::DWORD(cert == nullptr ? 0 : 1),
        .paCred = &cert,
        .dwFlags = SCH_USE_STRONG_CRYPTO,
        .cTlsParameters = 1,
        .pTlsParameters = &tls_params,
    };
    if (!server) {
        credentials.dwFlags |= SCH_CRED_NO_DEFAULT_CREDS;
        credentials.dwFlags |= verify_peer ? SCH_CRED_AUTO_CRED_VALIDATION : SCH_CRED_MANUAL_CRED_VALIDATION;
    }
    ::TimeStamp expiry;
    ::SECURITY_STATUS sstatus = ::AcquireCredentialsHandleW(nullptr, unisp_name.wc_str(), use, nullptr,
                                                            &credentials, nullptr, nullptr, &cred, &expiry);
    if (sstatus != SEC_E_OK) {
        report.error(u"error in AcquireCredentialsHandle: %s", WinErrorMessage(sstatus));
        return false;
    }
    report.debug(u"AcquireCredentialsHandle successful");
    return true;
}


//----------------------------------------------------------------------------
// Properly free and clear various types of handle.
//----------------------------------------------------------------------------

void ts::SafeFreeCredentials(::CredHandle& cred)
{
    if (cred.dwLower != 0 || cred.dwUpper != 0) {
        ::FreeCredentialsHandle(&cred);
        cred.dwLower = cred.dwUpper = 0;
    }
}

void ts::SafeDeleteSecurityContext(::CtxtHandle& ctx)
{
    if (ctx.dwLower != 0 || ctx.dwUpper != 0) {
        ::DeleteSecurityContext(&ctx);
        ctx.dwLower = ctx.dwUpper = 0;
    }
}
