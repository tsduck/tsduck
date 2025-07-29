//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsOpenSSL.h"
#include "tsCerrReport.h"
#include "tsEnvironment.h"

// Some OpenSSL macros use C-style casts and we need to disable warnings.
TS_LLVM_NOWARNING(old-style-cast)
TS_GCC_NOWARNING(old-style-cast)


//----------------------------------------------------------------------------
// Get a full version string for the OpenSSL library.
//----------------------------------------------------------------------------

ts::UString ts::OpenSSL::Version()
{
#if defined(TS_NO_OPENSSL)
    return UString();
#elif defined(OPENSSL_FULL_VERSION_STRING)
    // OpenSSL v3
    return UString::Format(u"OpenSSL %s (%s)", OpenSSL_version(OPENSSL_FULL_VERSION_STRING), OpenSSL_version(OPENSSL_CPU_INFO));
#else
    // OpenSSL v1
    return UString::FromUTF8(OpenSSL_version(OPENSSL_VERSION));
#endif
}


//----------------------------------------------------------------------------
// Check if environment variable TS_DEBUG_OPENSSL is defined.
//----------------------------------------------------------------------------

bool ts::OpenSSL::Debug()
{
    // Check once only.
    static const bool debug = !GetEnvironment(u"TS_DEBUG_OPENSSL").empty();
    return debug;
}


//----------------------------------------------------------------------------
// Display OpenSSL errors on standard error if environment variable TS_DEBUG_OPENSSL is defined.
//----------------------------------------------------------------------------

void ts::OpenSSL::DebugErrors()
{
#if !defined(TS_NO_OPENSSL)
    if (Debug()) {
        ERR_print_errors_fp(stderr);
    }
#endif
}


//----------------------------------------------------------------------------
// Report last errors from the OpenSSL library.
//----------------------------------------------------------------------------

void ts::OpenSSL::ReportErrors(Report& report, int severity)
{
    UStringList errors;
    GetErrors(errors);
    for (const auto& line : errors) {
        report.log(severity, line);
    }
}


//----------------------------------------------------------------------------
// Get last errors from the OpenSSL library.
//----------------------------------------------------------------------------

int ts::OpenSSL::GetErrorsCallback(const char* str, size_t len, void* u)
{
    ts::UStringList* list = reinterpret_cast<ts::UStringList*>(u);
    if (list != nullptr) {
        list->push_back(ts::UString::FromUTF8(str, len).toTrimmed(false));
    }
    return 0; // undocumented in OpenSSL man pages...
}

void ts::OpenSSL::GetErrors(UStringList& errors)
{
    errors.clear();

#if !defined(TS_NO_OPENSSL)
    // Get error messages in a list of strings.
    ERR_print_errors_cb(GetErrorsCallback, &errors);

    // The error messages are removed from the queue, explicitly apply debug display.
    if (Debug()) {
        for (const auto& line : errors) {
            CERR.error(line);
        }
    }
#endif
}


//----------------------------------------------------------------------------
// Create and configure a SSL_CTX context.
//----------------------------------------------------------------------------

SSL_CTX* ts::OpenSSL::CreateContext(bool server, bool verify_peer, Report& report)
{
    SSL_CTX* ssl_ctx = nullptr;

#if !defined(TS_NO_OPENSSL)

    ssl_ctx = ::SSL_CTX_new(server ? TLS_server_method() : TLS_client_method());
    if (ssl_ctx == nullptr) {
        report.error(u"error creating TLS %s context", server ? u"server" : u"client");
        OpenSSL::ReportErrors(report);
        return nullptr;
    }

    // Ignore unexpected EOF when the peer does not send close-notify.
    // Well-known servers such as google.com do this, so let's ignore it.
    ::SSL_CTX_set_options(ssl_ctx, SSL_OP_IGNORE_UNEXPECTED_EOF);

    // Accept only TLS 1.2 and 1.3, others are obsolete.
    ::SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_2_VERSION);

    // Check if the peer shall be verified.
    ::SSL_CTX_set_verify(ssl_ctx, verify_peer ? SSL_VERIFY_PEER : SSL_VERIFY_NONE, nullptr);

    // Use the default trusted certificate store.
    if (verify_peer && !::SSL_CTX_set_default_verify_paths(ssl_ctx)) {
        report.error(u"Failed to set the default trusted certificate store");
        OpenSSL::ReportErrors(report);
        ::SSL_CTX_free(ssl_ctx);
        return nullptr;
    }

#endif

    return ssl_ctx;
}


//----------------------------------------------------------------------------
// Base class for objects which must be terminated with OpenSSL.
//----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::OpenSSL::Controlled::Repo);

// The constructor registers the object into the repository of object to terminate.
ts::OpenSSL::Controlled::Controlled()
{
    Repo::Instance().registerObject(this);
}

// The destructor deregisters the object to avoid being called later.
ts::OpenSSL::Controlled::~Controlled()
{
    Repo::Instance().deregisterObject(this);
}

// Warning: the OpenSSL termination procedure can be called after the Repo destructor.
// This static boolean is set to true as long as the Repo singleton is alive.
bool ts::OpenSSL::Controlled::Repo::active = false;

// The private constructor of the Repo singleton registers exitHandler() to OpenSSL_atexit().
ts::OpenSSL::Controlled::Repo::Repo()
{
    active = true;
#if !defined(TS_NO_OPENSSL)
    OPENSSL_atexit(exitHandler);
#endif
}

// Destructor may be called before exitHandler().
// Make sure exitHandler() will not call a destructed object.
ts::OpenSSL::Controlled::Repo::~Repo()
{
    terminate();
    active = false;
}

// Register an instance.
void ts::OpenSSL::Controlled::Repo::registerObject(Controlled* obj)
{
    if (obj != nullptr) {
        std::lock_guard<std::mutex> lock(_mutex);
        _list.push_back(obj);
    }
}

// Deregister an instance.
void ts::OpenSSL::Controlled::Repo::deregisterObject(Controlled* obj)
{
    if (obj != nullptr) {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto it = _list.begin(); it != _list.end(); ) {
            if (*it == obj) {
                it = _list.erase(it);
            }
            else {
                ++it;
            }
        }
    }
}

// This method calls the terminate() method of all active instances of Controlled in
// reverse order of registration and deregisters them.
void ts::OpenSSL::Controlled::Repo::terminate()
{
    for (;;) {
        Controlled* obj = nullptr;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_list.empty()) {
                return;
            }
            obj = _list.back();
            _list.pop_back();
        }
        if (obj != nullptr) {
            obj->terminate();
        }
    }
}

// This static method is executed by OpenSSL termination procedure.
// Warning: may be called after Repo's destructor.
void ts::OpenSSL::Controlled::Repo::exitHandler()
{
    if (active) {
        Instance().terminate();
    }
}


//----------------------------------------------------------------------------
// A singleton which manages OpenSSL cryptographic providers.
//----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::OpenSSL::Providers);
ts::OpenSSL::Providers::Providers() {}

// Destructor is the same as terminate().
ts::OpenSSL::Providers::~Providers()
{
    Providers::terminate();
}

// Unload all providers. Must be idempotent.
void ts::OpenSSL::Providers::terminate()
{
#if defined(TS_OPENSSL_PROVIDERS)
    std::lock_guard<std::mutex> lock(_mutex);
    for (const auto& prov : _providers) {
        OSSL_PROVIDER_unload(prov.second);
    }
    _providers.clear();
#endif
}

// Load an OpenSSL provider if not yet loaded.
void ts::OpenSSL::Providers::load(const char* provider)
{
#if defined(TS_OPENSSL_PROVIDERS)
    const std::string name(provider != nullptr ? provider : "");
    if (!name.empty()) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_providers.contains(name)) {
            OSSL_PROVIDER* prov = OSSL_PROVIDER_load(nullptr, provider);
            if (prov != nullptr) {
                _providers[name] = prov;
            }
            else {
                OpenSSL::DebugErrors();
            }
        }
    }
#endif
}

// Get the properies string from an OpenSSL provider.
std::string ts::OpenSSL::Providers::Properties(const char* provider)
{
    return provider == nullptr || provider[0] == '\0' ? std::string() : std::string("provider=") + provider;
}
