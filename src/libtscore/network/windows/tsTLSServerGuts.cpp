//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// SSL/TLS server - Windows specific parts with SChannel.
//
//----------------------------------------------------------------------------

#include "tsTLSServer.h"
#include "tsWinUtils.h"

#include "tsBeforeStandardHeaders.h"
#include <wincrypt.h>
#include "tsAfterStandardHeaders.h"
#if defined(TS_MSC)
    #pragma comment(lib, "crypt32.lib")
#endif


//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::TLSServer::SystemGuts
{
    TS_NOCOPY(SystemGuts);
public:
    // Constructor and destructor.
    SystemGuts() = default;
    ~SystemGuts();

    // The certificate stores must remain open all the time, once open.
    // They are closed on termination of the singleton.
    class StoreRepo
    {
        TS_SINGLETON(StoreRepo);
    public:
        // Destructor: close opened stores.
        ~StoreRepo();
        // Get or open a certificate store.
        ::HCERTSTORE getStore(const UString& name, Report& report);
        // Get a certificate name.
        static UString GetCertName(::PCCERT_CONTEXT cert, ::DWORD type);
    private:
        std::mutex _mutex {};
        std::map<UString, ::HCERTSTORE> _stores {};
    };
};


//----------------------------------------------------------------------------
// System-specific constructor and destructor.
//----------------------------------------------------------------------------

void ts::TLSServer::allocateGuts()
{
    _guts = new SystemGuts;
}

void ts::TLSServer::deleteGuts()
{
    delete _guts;
}

ts::TLSServer::SystemGuts::~SystemGuts()
{
}


//----------------------------------------------------------------------------
// Certificate stores.
//----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::TLSServer::SystemGuts::StoreRepo);
ts::TLSServer::SystemGuts::StoreRepo::StoreRepo() {}

// Destructor: close opened stores.
ts::TLSServer::SystemGuts::StoreRepo::~StoreRepo()
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
::HCERTSTORE ts::TLSServer::SystemGuts::StoreRepo::getStore(const UString& name, Report& report)
{
    // Resolve certificate store name.
    ::DWORD flags = CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG;
    if (name == u"user") {
        flags |= CERT_SYSTEM_STORE_CURRENT_USER;
    }
    else if (name == u"system") {
        flags |= CERT_SYSTEM_STORE_LOCAL_MACHINE;
    }
    else {
        report.error(u"invalid certificate store name \"%s\"", name);
        return nullptr;
    }

    // Get the store in the map, under lock protection.
    std::lock_guard<std::mutex> lock(_mutex);
    if (_stores.contains(name)) {
        return _stores[name];
    }
    else {
        report.debug(u"opening certificate store \"%s\"", name);
        ::HCERTSTORE hcs = ::CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, flags, L"My");
        if (hcs != nullptr) {
            return _stores[name] = hcs;
        }
        else {
            report.error(u"error opening certificate store \"%s\": %s", name, WinErrorMessage(::GetLastError()));
            return nullptr;
        }
    }
}

// Get a certificate name.
ts::UString ts::TLSServer::SystemGuts::StoreRepo::GetCertName(::PCCERT_CONTEXT cert, ::DWORD type)
{
    ::DWORD size = ::CertGetNameStringW(cert, type, 0, nullptr, nullptr, 0);
    std::vector<::WCHAR> name(std::max<size_t>(1, size));
    size = ::CertGetNameStringW(cert, type, 0, nullptr, name.data(), ::DWORD(name.size()));
    return UString(name.data(), std::max<size_t>(1, std::min<size_t>(name.size(), size)) - 1);
}


//----------------------------------------------------------------------------
// Start the server
//----------------------------------------------------------------------------

bool ts::TLSServer::listen(int backlog, Report& report)
{
    // We need a certificate.
    const UString cert_name(getCertificatePath());
    if (cert_name.empty()) {
        report.error(u"no certificate set in TLS server");
        return false;
    }

    // Get the certificate store.
    ::HCERTSTORE store = SystemGuts::StoreRepo::Instance().getStore(getCertificateStore(), report);
    if (store == nullptr) {
        return false;
    }

    // Search the certificate in the store. Only consider certificates with a private key.
    ::PCCERT_CONTEXT cert = nullptr;
    while ((cert = ::CertFindCertificateInStore(store, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_HAS_PRIVATE_KEY, nullptr, cert)) != nullptr &&
           SystemGuts::StoreRepo::GetCertName(cert, CERT_NAME_FRIENDLY_DISPLAY_TYPE) != cert_name &&
           SystemGuts::StoreRepo::GetCertName(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE) != cert_name &&
           SystemGuts::StoreRepo::GetCertName(cert, CERT_NAME_DNS_TYPE) != cert_name) {
    }
    if (cert == nullptr) {
        // Certificate not found: not found or error?
        const ::DWORD err = ::GetLastError();
        if (err == CRYPT_E_NOT_FOUND) {
            report.error(u"certificate \"%s\" not found", cert_name);
        }
        else {
            report.error(u"error searching certificate \"%s\": %s", cert_name, WinErrorMessage(err));
        }
        return false;
    }

    //@@@@ TO BE CONTINUED
    ::CertFreeCertificateContext(cert);
    report.error(u"not yet implemented");
    return false;

    // Create the TCP server.
    //@@@@ return SuperClass::listen(backlog, report);
}


//----------------------------------------------------------------------------
// Wait for a TLS client.
//----------------------------------------------------------------------------

bool ts::TLSServer::acceptTLS(TLSConnection& client, IPSocketAddress& addr, Report& report)
{
    // Accept one TCP client.
    //@@@@ if (!SuperClass::accept(client, addr, report)) {
    //@@@@     return false;
    //@@@@ }

    //@@@@ TO BE CONTINUED
    report.error(u"not yet implemented");
    return false;
}


//----------------------------------------------------------------------------
// Close the server resources.
//----------------------------------------------------------------------------

bool ts::TLSServer::close(Report& report)
{
    //@@@@ TO BE CONTINUED
    report.error(u"not yet implemented");

    return SuperClass::close(report);
}
