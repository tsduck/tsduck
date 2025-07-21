//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup libtscore unix
//!  OpenSSL utilities for UNIX systems.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsSeverity.h"

//! @cond nodoxygen
#if !defined(TS_NO_OPENSSL)
    #include "tsBeforeStandardHeaders.h"
    #include <openssl/opensslv.h>
    #include <openssl/evp.h>
    #include <openssl/err.h>
    #include <openssl/ssl.h>
    #if !defined(OPENSSL_VERSION_MAJOR) // before v3
        #define OPENSSL_VERSION_MAJOR (OPENSSL_VERSION_NUMBER >> 28)
    #endif
    #if !defined(OPENSSL_VERSION_MINOR) // before v3
        #define OPENSSL_VERSION_MINOR ((OPENSSL_VERSION_NUMBER >> 20) & 0xFF)
    #endif
    #if !defined(OPENSSL_VERSION_PATCH) // before v3
        #define OPENSSL_VERSION_PATCH ((OPENSSL_VERSION_NUMBER >> 4) & 0xFFFF)
    #endif
    #if OPENSSL_VERSION_MAJOR >= 3
        // Starting with OpenSSL 3.0, algorithms are stored in providers.
        #define TS_OPENSSL_PROVIDERS 1
        #include <openssl/core_names.h>
        #include <openssl/provider.h>
    #elif !defined(OPENSSL_atexit)
        // OpenBSD uses LibreSSL 4.0.0 which says it is OpenSSL 2.0.0 but
        // emulates OpenSSL v3, except that OPENSSL_atexit is not available.
        #define OPENSSL_atexit atexit
    #endif
    #include "tsAfterStandardHeaders.h"

#else

    // No support for external cryptographic library.
    #define TS_NO_CRYPTO_LIBRARY 1

    // Generic error message.
    #define TS_NO_OPENSSL_MESSAGE u"This version of TSDuck was compiled without OpenSSL"

    // Placeholders for OpenSSL types.
    struct SSL_CTX;
    struct SSL;

#endif
//! @endcond

namespace ts {

    class Report;

    //!
    //! Utilities for the OpenSSL library.
    //! Note that since OpenSSL 1.1.0, no explicit initialization or cleanup is required.
    //! @ingroup unix crypto
    //!
    class TSCOREDLL OpenSSL
    {
    public:
        //!
        //! Get a full version string for the OpenSSL library.
        //! @return The version string.
        //!
        static UString Version();

        //!
        //! Get last errors from the OpenSSL library.
        //! The error messages are removed from the OpenSSL error message queue.
        //! @param [out] errors List of error messages.
        //!
        static void GetErrors(UStringList& errors);

        //!
        //! Report last errors from the OpenSSL library.
        //! The error messages are removed from the OpenSSL error message queue.
        //! @param [out] report Where to report error messages.
        //! @param [in] severity Severity level.
        //!
        static void ReportErrors(Report& report, int severity = Severity::Error);

        //!
        //! Display OpenSSL errors on standard error if environment variable TS_DEBUG_OPENSSL is defined.
        //!
        static void DebugErrors();

        //!
        //! Check if environment variable TS_DEBUG_OPENSSL is defined.
        //! @return True if environment variable TS_DEBUG_OPENSSL is defined.
        //!
        static bool Debug();

        //!
        //! Create and configure a SSL_CTX context.
        //! @param [in] server True for server side, false for client side.
        //! @param [in] verify_peer Verify the certificate of the peer.
        //! @param [in,out] report Where to report errors.
        //! @return The new SSL_CTX, nullptr on error.
        //!
        static SSL_CTX* CreateContext(bool server, bool verify_peer, Report& report);

        //!
        //! Base class for objects which must be terminated with OpenSSL.
        //! When the application terminates, OpenSSL does its own cleanup.
        //! After this cleanup, no OpenSSL operation is possible. I can lead to crashes.
        //! A class which uses OpenSSL shall terminate all its processing no later than
        //! the OpenSSL cleanup. This is enforced by this base class.
        //!
        class TSCOREDLL Controlled
        {
            TS_NOCOPY(Controlled);
        public:
            //!
            //! Constructor.
            //!
            Controlled();
            //!
            //! Destructor.
            //!
            virtual ~Controlled();
            //!
            //! This method terminates anything about OpenSSL which must be cleaned up.
            //! This method must be idempotent and must be called in the subclass destructor.
            //!
            virtual void terminate() = 0;

        private:
            // This internal class is a repository of all active instances of Controlled.
            // The constructor and destructor of Controlled perform the registration / deregistration.
            class TSCOREDLL Repo
            {
                TS_SINGLETON(Repo);
            public:
                // Register / deregister an instance.
                void registerObject(Controlled*);
                void deregisterObject(Controlled*);

            private:
                // List of registered instances, under protection of a mutex.
                std::mutex _mutex {};
                std::list<Controlled*> _list {};

                // This method calls the terminate() method of all active instances of Controlled in
                // reverse order of registration and deregisters them.
                void terminate();

                // This static method is executed by OpenSSL termination procedure.
                // It calls the instance's terminate().
                [[maybe_unused]] static void exitHandler();

                // Warning: the OpenSSL termination procedure can be called after the Repo destructor.
                // This static boolean is set to true as long as the Repo singleton is alive.
                static bool active;
                ~Repo();
            };
        };

        //!
        //! A singleton which manages OpenSSL cryptographic providers.
        //!
        class Providers : public OpenSSL::Controlled
        {
            TS_SINGLETON(Providers);
        public:
            // Implementation of Controlled.
            virtual void terminate() override;
            virtual ~Providers() override;

            //!
            //! Load an OpenSSL provider if not yet loaded.
            //! @param [in] provider Provider name.
            //!
            void load(const char* provider);

            //!
            //! Get the properies string from an OpenSSL provider.
            //! @param [in] provider Provider name.
            //! @return Corresponding properties string.
            //!
            static std::string Properties(const char* provider);

        private:
        #if defined(TS_OPENSSL_PROVIDERS)
            std::mutex _mutex {};
            std::map<std::string,OSSL_PROVIDER*> _providers {};
        #endif
        };

    private:
        // Callback function for ERR_print_errors_cb().
        [[maybe_unused]] static int GetErrorsCallback(const char* str, size_t len, void* u);
    };
}
