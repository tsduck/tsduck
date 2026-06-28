//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// SSL/TLS certificate - Windows specific parts.
//
//----------------------------------------------------------------------------

#include "tsTLSCertificate.h"
#include "tsEnvironment.h"
#include "tsSysUtils.h"
#include "tsSysInfo.h"
#include "tsNames.h"
#include "tsWinTLS.h"


//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::TLSCertificate::SystemGuts
{
    TS_NOBUILD_NOCOPY(SystemGuts);
public:
    TLSCertificate&      tls;
    ::PCCERT_CONTEXT     cert = nullptr;
    ::NCRYPT_PROV_HANDLE provider = 0;
    ::NCRYPT_KEY_HANDLE  key = 0;

    // Constructor and destructor.
    SystemGuts(TLSCertificate& c) : tls(c) {}
    ~SystemGuts();

    // Reset the guts.
    void reset();

    // Get the name of a certificate name for a given type (CERT_NAME_FRIENDLY_DISPLAY_TYPE, CERT_NAME_xxx).
    static UString GetCertificateName(::PCCERT_CONTEXT cert, ::DWORD type);

    // Repository of Windows certificate stores.
    // The certificate stores must remain open all the time, once open.
    // They are closed on termination of the singleton.
    class CertStoreRepository
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


//----------------------------------------------------------------------------
// System-specific constructor and destructor.
//----------------------------------------------------------------------------

void ts::TLSCertificate::allocateGuts()
{
    _guts = new SystemGuts(*this);
}

void ts::TLSCertificate::deleteGuts()
{
    delete _guts;
}

ts::TLSCertificate::SystemGuts::~SystemGuts()
{
    reset();
}

void ts::TLSCertificate::SystemGuts::reset()
{
    if (cert != nullptr) {
        ::CertFreeCertificateContext(cert);
        cert = nullptr;
    }
    if (key != 0) {
        const ::SECURITY_STATUS status = ::NCryptDeleteKey(key, NCRYPT_SILENT_FLAG);
        key = 0;
        if (!SysSuccess(status)) {
            tls.report().error(u"failed to delete temporary RSA key: %s", WinErrorMessage(status));
        }
    }
    if (provider != 0) {
        ::NCryptFreeObject(provider);
        provider = 0;
    }
}

void ts::TLSCertificate::reset()
{
    _guts->reset();
}

void* ts::TLSCertificate::getCertificate() const
{
    return const_cast<::PCERT_CONTEXT>(_guts->cert);
}


//----------------------------------------------------------------------------
// Get a certificate name.
//----------------------------------------------------------------------------

ts::UString ts::TLSCertificate::SystemGuts::GetCertificateName(::PCCERT_CONTEXT cert, ::DWORD type)
{
    ::DWORD size = ::CertGetNameStringW(cert, type, 0, nullptr, nullptr, 0);
    std::vector<::WCHAR> name(std::max<size_t>(1, size));
    size = ::CertGetNameStringW(cert, type, 0, nullptr, name.data(), ::DWORD(name.size()));
    return UString(name.data(), std::max<size_t>(1, std::min<size_t>(name.size(), size)) - 1);
}


//-----------------------------------------------------------------------------
// Create an ephemeral self-signed certificate.
//-----------------------------------------------------------------------------

bool ts::TLSCertificate::createEphemeralCertificate(size_t rsa_bits)
{
    // Clear previous certificate.
    _guts->reset();

    // Various resources to cleanup at the end.
    ::PBYTE eku_data = nullptr;  // Allocated by LocalAlloc in CryptEncodeObjectEx().
    ::PBYTE san_data = nullptr;  // Allocated by LocalAlloc in CryptEncodeObjectEx().
    ::PBYTE ku_data = nullptr;   // Allocated by LocalAlloc in CryptEncodeObjectEx().
    bool success = false;        // Set at the end.

    // Use a "do {} while (false)" pattern to allow early "break" and jump to cleanup in case of error.
    do {
        // Open the default crypto provider.
        ::SECURITY_STATUS status = ::NCryptOpenStorageProvider(&_guts->provider, MS_KEY_STORAGE_PROVIDER, 0);
        if (!SysSuccess(status)) {
            report().error(u"failed to open key storage provider: %s", WinErrorMessage(status));
            break;
        }

        // Purely ephemeral keys, staying in memory without persistent storage, are rejected by SChannel.
        // AcquireCredentialsHandle returns error SEC_E_NO_CREDENTIALS. We need to give a name to the key,
        // let it be stored, and then explicitly delete it when we no longer need it.
        UString key_name;
        key_name.format(u"ts-temp-key-%d-%d", ::GetCurrentProcessId(), ::GetTickCount64());

        // Start the key definition (not created yet).
        status = ::NCryptCreatePersistedKey(_guts->provider, &_guts->key, BCRYPT_RSA_ALGORITHM, key_name.wc_str(), 0, NCRYPT_OVERWRITE_KEY_FLAG);
        if (!SysSuccess(status)) {
            report().error(u"failed to create an ephemeral RSA key: %s", WinErrorMessage(status));
            break;
        }

        // Set the key size property (in bits).
        ::DWORD key_len = ::DWORD(rsa_bits);
        status = ::NCryptSetProperty(_guts->key, NCRYPT_LENGTH_PROPERTY, ::PBYTE(&key_len), ::DWORD(sizeof(key_len)), NCRYPT_SILENT_FLAG);
        if (!SysSuccess(status)) {
            report().error(u"failed to set ephemeral RSA key size: %s", WinErrorMessage(status));
            break;
        }

        // Private key usage: decrypt and sign.
        ::DWORD key_usage = NCRYPT_ALLOW_SIGNING_FLAG | NCRYPT_ALLOW_DECRYPT_FLAG;
        status = ::NCryptSetProperty(_guts->key, NCRYPT_KEY_USAGE_PROPERTY, ::PBYTE(&key_usage), ::DWORD(sizeof(key_usage)), NCRYPT_SILENT_FLAG);
        if (!SysSuccess(status)) {
            report().error(u"failed to set ephemeral RSA key usage: %s", WinErrorMessage(status));
            break;
        }

        // Now create the key.
        status = ::NCryptFinalizeKey(_guts->key, NCRYPT_SILENT_FLAG);
        if (!SysSuccess(status)) {
            report().error(u"failed to create ephemeral RSA key: %s", WinErrorMessage(status));
            break;
        }

        // Certificate subject string ("CN=hostname").
        // Encode in two steps: first step returns the required size, second step encodes in a properly sized buffer.
        UString subject_string = u"CN=" + Subject();
        std::vector<::BYTE> subject_data;
        ::DWORD subject_size = 0;
        bool ok = ::CertStrToNameW(X509_ASN_ENCODING, subject_string.wc_str(), CERT_X500_NAME_STR, nullptr, nullptr, &subject_size, nullptr);
        if (ok) {
            subject_data.resize(subject_size);
            ok = ::CertStrToNameW(X509_ASN_ENCODING, subject_string.wc_str(), CERT_X500_NAME_STR, nullptr, subject_data.data(), &subject_size, nullptr);
        }
        if (!ok) {
            report().error(u"failed to encode CN string for certificate: %s", WinErrorMessage(LastSysErrorCode()));
            break;
        }
        ::CERT_NAME_BLOB subject_blob {.cbData = subject_size, .pbData = subject_data.data()};

        // Enhanced Key Usage (EKU) certificate extension.
        ::LPSTR server_auth_oid = const_cast<::LPSTR>(szOID_PKIX_KP_SERVER_AUTH);
        ::CERT_ENHKEY_USAGE eku {.cUsageIdentifier = 1, .rgpszUsageIdentifier = &server_auth_oid};
        ::DWORD eku_size = 0;
        if (!CryptEncodeObjectEx(X509_ASN_ENCODING, szOID_ENHANCED_KEY_USAGE, &eku, CRYPT_ENCODE_ALLOC_FLAG, nullptr, &eku_data, &eku_size)) {
            report().error(u"failed to encode Enhanced Key Usage (EKU) for certificate: %s", WinErrorMessage(LastSysErrorCode()));
            break;
        }

        // Subject Alternative Names (SAN) certificate extension.
        UString local_host = u"localhost";
        UString host_name = SysInfo::Instance().hostName();
        std::vector<::CERT_ALT_NAME_ENTRY> san_names {
            {.dwAltNameChoice = CERT_ALT_NAME_DNS_NAME, .pwszDNSName = host_name.wc_str()},
            {.dwAltNameChoice = CERT_ALT_NAME_DNS_NAME, .pwszDNSName = local_host.wc_str()},
        };
        ::CERT_ALT_NAME_INFO san {.cAltEntry = ::DWORD(san_names.size()), .rgAltEntry = san_names.data()};
        ::DWORD san_size = 0;
        if (!CryptEncodeObjectEx(X509_ASN_ENCODING, szOID_SUBJECT_ALT_NAME2, &san, CRYPT_ENCODE_ALLOC_FLAG, nullptr, &san_data, &san_size)) {
            report().error(u"failed to encode Subject Alternative Names (SAN) for certificate: %s", WinErrorMessage(LastSysErrorCode()));
            break;
        }

        // Key Usage (KU) certificate extension.
        ::BYTE ku_value = CERT_DIGITAL_SIGNATURE_KEY_USAGE | CERT_KEY_ENCIPHERMENT_KEY_USAGE;
        ::CRYPT_BIT_BLOB ku {.cbData = 1, .pbData = &ku_value, .cUnusedBits = 0};
        ::DWORD ku_size = 0;
        if (!CryptEncodeObjectEx(X509_ASN_ENCODING, szOID_KEY_USAGE, &ku, CRYPT_ENCODE_ALLOC_FLAG, nullptr, &ku_data, &ku_size)) {
            report().error(u"failed to encode Key Usage (KU) for certificate: %s", WinErrorMessage(LastSysErrorCode()));
            break;
        }

        // Certificate extensions.
        std::vector<::CERT_EXTENSION> exts {
            {.pszObjId = const_cast<::LPSTR>(szOID_ENHANCED_KEY_USAGE), .fCritical = false, .Value = {eku_size, eku_data}},
            {.pszObjId = const_cast<::LPSTR>(szOID_SUBJECT_ALT_NAME2), .fCritical = false, .Value = {san_size, san_data}},
            {.pszObjId = const_cast<::LPSTR>(szOID_KEY_USAGE), .fCritical = true, .Value = {ku_size, ku_data}},
        };
        ::CERT_EXTENSIONS cert_exts {.cExtension = ::DWORD(exts.size()), .rgExtension = exts.data()};

        // Provider info, required to use the key in SChannel.
        ::CRYPT_KEY_PROV_INFO prov_info {
            .pwszContainerName = key_name.wc_str(),
            .pwszProvName = const_cast<::LPWSTR>(MS_KEY_STORAGE_PROVIDER),
            .dwProvType = 0,
            .dwFlags = NCRYPT_SILENT_FLAG,
            .cProvParam = 0,
            .rgProvParam = nullptr,
            .dwKeySpec = 0,
        };

        // Use PKCS#1 v1.5 with SHA-256 as signature algorithm.
        // We could use szOID_RSA_SSA_PSS but we would need to add CRYPT_RSA_SSA_PSS_PARAMETERS.
        // Since this is a self-signed certificate for tests, no need to use complicated structures.
        ::CRYPT_ALGORITHM_IDENTIFIER sig_algo {.pszObjId = const_cast<::LPSTR>(szOID_RSA_SHA256RSA), .Parameters = {0, nullptr}};

        // Create the certificate for the key and self-sign it.
        // The default validity dates are now to one year from now (both nullptr here).
        _guts->cert = ::CertCreateSelfSignCertificate(0, &subject_blob, 0, &prov_info, &sig_algo, nullptr, nullptr, &cert_exts);
        if (_guts->cert == nullptr) {
            report().error(u"failed to create a self-signed certificate: %s", WinErrorMessage(LastSysErrorCode()));
            break;
        }

        // End of initialization sequence. Inform the cleanup phase that we succeeded.
        success = true;

    } while (false);

    // Cleanup intermediate storage.
    if (eku_data != nullptr) {
        ::LocalFree(eku_data);
    }
    if (san_data != nullptr) {
        ::LocalFree(san_data);
    }
    if (ku_data != nullptr) {
        ::LocalFree(ku_data);
    }

    // Cleanup partially built resources in case of failure.
    if (!success) {
        reset();
    }
    return success;
}


//-----------------------------------------------------------------------------
// Load a certificate from a store.
//-----------------------------------------------------------------------------

bool ts::TLSCertificate::loadCertificate(const UString& certificate_path, const UString& key_path, const UString& store_name)
{
    // Clear previous certificate.
    _guts->reset();

    // We need a certificate name.
    if (certificate_path.empty()) {
        report().error(u"no TLS certificate is specified");
        return false;
    }

    // Get the certificate store.
    ::HCERTSTORE store = SystemGuts::CertStoreRepository::Instance().getStore(store_name, report());
    if (store == nullptr) {
        return false;
    }

    // Search the certificate in the store. Only consider certificates with a private key.
    while ((_guts->cert = ::CertFindCertificateInStore(store, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_HAS_PRIVATE_KEY, nullptr, _guts->cert)) != nullptr) {
        if (SystemGuts::GetCertificateName(_guts->cert, CERT_NAME_FRIENDLY_DISPLAY_TYPE) == certificate_path ||
            SystemGuts::GetCertificateName(_guts->cert, CERT_NAME_SIMPLE_DISPLAY_TYPE) == certificate_path ||
            SystemGuts::GetCertificateName(_guts->cert, CERT_NAME_DNS_TYPE) == certificate_path)
        {
            report().debug(u"found certificate \"%s\"", certificate_path);
            return true;
        }
    }

    // Certificate not found: not found or error?
    const ::DWORD err = ::GetLastError();
    if (err == CRYPT_E_NOT_FOUND) {
        report().error(u"certificate \"%s\" not found", certificate_path);
    }
    else {
        report().error(u"error searching certificate \"%s\": %s", certificate_path, WinErrorMessage(err));
    }
    return false;
}


//----------------------------------------------------------------------------
// Certificate stores.
//----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::TLSCertificate::SystemGuts::CertStoreRepository);
ts::TLSCertificate::SystemGuts::CertStoreRepository::CertStoreRepository() {}

// Destructor: close opened stores.
ts::TLSCertificate::SystemGuts::CertStoreRepository::~CertStoreRepository()
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
::HCERTSTORE ts::TLSCertificate::SystemGuts::CertStoreRepository::getStore(const UString& store_name, Report& report)
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
