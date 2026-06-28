//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// SSL/TLS certificate - UNIX specific parts with OpenSSL.
//
//----------------------------------------------------------------------------

#include "tsTLSCertificate.h"
#include "tsSysInfo.h"
#include "tsOpenSSL.h"

// Some OpenSSL macros use C-style casts and we need to disable warnings.
TS_LLVM_NOWARNING(old-style-cast)
TS_GCC_NOWARNING(old-style-cast)


//----------------------------------------------------------------------------
// Stubs when OpenSSL is not available.
//----------------------------------------------------------------------------

#if defined(TS_NO_OPENSSL)

#define TS_NOT_IMPL { report().error(TS_NO_OPENSSL_MESSAGE); return false; }

class ts::TLSCertificate::SystemGuts {};
void ts::TLSCertificate::allocateGuts() { _guts = new SystemGuts; }
void ts::TLSCertificate::deleteGuts() { delete _guts; }
void ts::TLSCertificate::reset() {}
void* ts::TLSCertificate::getCertificate() const { return nullptr; }
bool ts::TLSCertificate::createEphemeralCertificate(size_t) TS_NOT_IMPL
bool ts::TLSCertificate::loadCertificate(const UString&, const UString&, const UString&) TS_NOT_IMPL

#else

//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::TLSCertificate::SystemGuts: public OpenSSL::Controlled
{
    TS_NOBUILD_NOCOPY(SystemGuts);
public:
    TLSCertificate& cert;
    SSL_CTX* ssl_ctx = nullptr;

    // Constructor and destructor.
    SystemGuts(TLSCertificate& c) : cert(c) {}
    virtual ~SystemGuts() override;

    // Implementation of OpenSSL::Controlled.
    virtual void terminate() override;

    // Create the SSL server context.
    bool initServerContext();

    // Add to certificate 'cert' X.509v3 extension id 'nid'
    bool addExtension(X509* cert, int nid, const char* value);
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
    SystemGuts::terminate();
}

void ts::TLSCertificate::SystemGuts::terminate()
{
    if (ssl_ctx != nullptr) {
        SSL_CTX_free(ssl_ctx);
        ssl_ctx = nullptr;
    }
}

void ts::TLSCertificate::reset()
{
    _guts->terminate();
}

void* ts::TLSCertificate::getCertificate() const
{
    return _guts->ssl_ctx;
}


//-----------------------------------------------------------------------------
// Add to certificate 'x509' X.509v3 extension id 'nid'
//-----------------------------------------------------------------------------

bool ts::TLSCertificate::SystemGuts::addExtension(X509* x509, int nid, const char* value)
{
    X509V3_CTX ctx {};

    // No external configuration database, use only literal values without reference to other sections.
    TS_PUSH_WARNING()
    TS_LLVM_NOWARNING(extra-semi-stmt) // OpenSSL: X509V3_set_ctx_nodb macro is defined with extraneous ';'
    TS_GCC_NOWARNING(extra-semi-stmt)
    X509V3_set_ctx_nodb(&ctx);
    TS_POP_WARNING()

    // Self-signed: issuer = subject = current certificate.
    X509V3_set_ctx(&ctx, x509, x509, nullptr, nullptr, 0);
    X509_EXTENSION* ext = X509V3_EXT_nconf_nid(nullptr, &ctx, nid, value);
    if (ext == nullptr) {
        return false;
    }

    const bool success = X509_add_ext(x509, ext, -1);
    X509_EXTENSION_free(ext);
    return success;
}


//-----------------------------------------------------------------------------
// Create the SSL server context.
//-----------------------------------------------------------------------------

bool ts::TLSCertificate::SystemGuts::initServerContext()
{
    // Clear previous certificate.
    terminate();

    // Create SSL server context.
    if ((ssl_ctx = SSL_CTX_new(TLS_server_method())) == nullptr) {
        cert.report().error(u"error creating OpenSSL context");
        return false;
    }

    // Ignore unexpected EOF when the peer does not send close-notify.
    // Well-known servers such as google.com do this, so let's ignore it.
    SSL_CTX_set_options(ssl_ctx, SSL_OP_IGNORE_UNEXPECTED_EOF);

    // Accept only TLS 1.2 and 1.3, others are obsolete.
    SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_2_VERSION);
    return true;
}


//-----------------------------------------------------------------------------
// Create an ephemeral self-signed certificate.
//-----------------------------------------------------------------------------

bool ts::TLSCertificate::createEphemeralCertificate(size_t rsa_bits)
{
    EVP_PKEY*  pkey = nullptr;
    X509*      x509 = nullptr;
    const long cert_validity_seconds = 365 * 24 * 3600; // one year
    bool       success = false;

    // Use a "do {} while (false)" pattern to allow early "break" and jump to cleanup in case of error.
    do {
        // Create SSL server context.
        if (!_guts->initServerContext()) {
            break;
        }

        // Ignore unexpected EOF when the peer does not send close-notify.
        // Well-known servers such as google.com do this, so let's ignore it.
        SSL_CTX_set_options(_guts->ssl_ctx, SSL_OP_IGNORE_UNEXPECTED_EOF);

        // Accept only TLS 1.2 and 1.3, others are obsolete.
        SSL_CTX_set_min_proto_version(_guts->ssl_ctx, TLS1_2_VERSION);

        // Generate ephemeral RSA key.
        TS_PUSH_WARNING()
        TS_LLVM_NOWARNING(old-style-cast)
        TS_GCC_NOWARNING(old-style-cast)
        if ((pkey = EVP_RSA_gen(rsa_bits)) == nullptr) {
            report().error(u"error generating %d-bit RSA key", rsa_bits);
            break;
        }
        TS_POP_WARNING()

        // Create certificate.
        if ((x509 = X509_new()) == nullptr) {
            report().error(u"error initializing X509 certificate");
            break;
        }

        // Version 2 means X.509 v3 certificate, required for extensions.
        if (!X509_set_version(x509, 2)) {
            report().error(u"error setting X509 certificate version");
            break;
        }

        // Set certificate serial number to 1 (not significant in ephemeral certificate).
        if (!ASN1_INTEGER_set(X509_get_serialNumber(x509), 1)) {
            report().error(u"error setting X509 serial number");
            break;
        }

        // Certificate validity, starting now.
        if (X509_gmtime_adj(X509_getm_notBefore(x509), 0) == nullptr || X509_gmtime_adj(X509_getm_notAfter(x509), cert_validity_seconds) == nullptr) {
            report().error(u"error setting certificate validity dates");
            break;
        }

        // Add public key to certificate.
        if (!X509_set_pubkey(x509, pkey)) {
            report().error(u"error adding public key to certificate");
            break;
        }

        // Set subject name (CN) and issuer to host name (this is a sel-signed certificate).
        UString host_name(SysInfo::Instance().hostName());
        X509_NAME* subject_name = X509_get_subject_name(x509); // internal pointer inside x509
        if (!X509_NAME_add_entry_by_txt(subject_name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char*>(host_name.toUTF8().c_str()), -1, -1, 0)) {
            report().error(u"error setting subject of certificate");
            break;
        }
        if (!X509_set_issuer_name(x509, subject_name)) {
            report().error(u"error setting issuer of certificate");
            break;
        }

        // Set "basicConstraints" extension. Set as critical.
        if (!_guts->addExtension(x509, NID_basic_constraints, "critical,CA:FALSE")) {
            report().error(u"error setting basic constraints of certificate");
            break;
        }

        // keyUsage : digitalSignature (sign) + keyEncipherment (decrypt).
        if (!_guts->addExtension(x509, NID_key_usage, "critical,digitalSignature,keyEncipherment")) {
            report().error(u"error setting key usage of certificate");
            break;
        }

        // extendedKeyUsage : TLS server authentication.
        if (!_guts->addExtension(x509, NID_ext_key_usage, "serverAuth")) {
            report().error(u"error setting extended key usage of certificate");
            break;
        }

        // subjectAltName : host name, local address
        UString alt_names;
        alt_names.format(u"DNS:%s, DNS:localhost, IP:127.0.0.1", host_name);
        if (!_guts->addExtension(x509, NID_subject_alt_name, alt_names.toUTF8().c_str())) {
            report().error(u"error setting alternate names of certificate");
            break;
        }

        // Self-sign the certificate. Use SHA-256 with PKCS#1 v1.5.
        if (!X509_sign(x509, pkey, EVP_sha256())) {
            report().error(u"error signing certificate");
            break;
        }

        // Load the certificate in the SSL context. The context increments the reference count
        // of the certificate and the key. Therefore, we need to free our reference before returning.
        if (SSL_CTX_use_certificate(_guts->ssl_ctx, x509) != 1) {
            report().error(u"error loading self-signed certificate in SSL context");
            break;
        }
        if (SSL_CTX_use_PrivateKey(_guts->ssl_ctx, pkey) != 1) {
            report().error(u"error loading private key in SSL context");
            break;
        }

        // Check key / certificate consistency.
        if (SSL_CTX_check_private_key(_guts->ssl_ctx) != 1) {
            report().error(u"invalid key / certificate consistency");
            break;
        }

        // End of initialization sequence.
        success = true;
    } while (false);

    // Free these resources. In case of success, one reference is kept in SSL_CTX. Otherwise, cleanup.
    if (x509 != nullptr) {
        X509_free(x509);
    }
    if (pkey != nullptr) {
        EVP_PKEY_free(pkey);
    }

    // Cleanup partially built resources in case of failure.
    if (!success) {
        OpenSSL::ReportErrors(report());
        _guts->terminate();
    }
    return success;
}


//-----------------------------------------------------------------------------
// Load a certificate from a store.
//-----------------------------------------------------------------------------

bool ts::TLSCertificate::loadCertificate(const UString& certificate_path, const UString& key_path, const UString& store_name)
{
    // We need a certificate and a key.
    if (certificate_path.empty()) {
        report().error(u"no certificate set in TLS server");
        return false;
    }
    if (key_path.empty()) {
        report().error(u"no private key set in TLS server");
        return false;
    }

    // Use a "do {} while (false)" pattern to allow early "break" and jump to cleanup in case of error.
    bool success = false;
    do {
        // Create SSL server context.
        if (!_guts->initServerContext()) {
            break;
        }

        // Load certificate file (public key).
        if (SSL_CTX_use_certificate_file(_guts->ssl_ctx, certificate_path.toUTF8().c_str(), SSL_FILETYPE_PEM) <= 0) {
            report().error(u"error loading TLS certificate file %s", certificate_path);
            break;
        }

        // Load private key file.
        if (SSL_CTX_use_PrivateKey_file(_guts->ssl_ctx, key_path.toUTF8().c_str(), SSL_FILETYPE_PEM) <= 0) {
            report().error(u"error loading TLS private key file %s", key_path);
            break;
        }

        // End of initialization sequence.
        success = true;
    } while (false);

    // Cleanup partially built resources in case of failure.
    if (!success) {
        OpenSSL::ReportErrors(report());
        _guts->terminate();
    }
    return success;
}

#endif // TS_NO_OPENSSL
