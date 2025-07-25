//
// SSL/TLS - Windows specific headers.
//
//----------------------------------------------------------------------------

#include "tsWinUtils.h"

#define SECURITY_WIN32 1           // used by sspi.h (versus SECURITY_KERNEL)
#define SCHANNEL_USE_BLACKLISTS 1  // for SCH_CREDENTIALS

#include "tsBeforeStandardHeaders.h"
#include <subauth.h>
#include <sspi.h>
#include <schannel.h>
#include "tsAfterStandardHeaders.h"

#if defined(TS_MSC)
    #pragma comment(lib, "crypt32.lib")
    #pragma comment(lib, "secur32.lib")
#endif

namespace ts {
    //!
    //! Get a certificate.
    //! @param [in] store_name Name of certificate store. One of "system", "user".
    //! @param [in] cert_name Name of the certificate (friendly name or subject name or DNS name).
    //! @param [in,out] report Where to report errors.
    //! @return A PCCERT_CONTEXT handle to the certificate. Must be freeed using CertFreeCertificateContext().
    //! Return nullptr on error.
    //!
    TSCOREDLL ::PCCERT_CONTEXT GetCertificate(const UString& store_name, const UString& cert_name, Report& report);

    //!
    //! Get the name of a certificate name for a given type.
    //! @param [in] cert Certificate handle.
    //! @param [in] type Type of name (CERT_NAME_FRIENDLY_DISPLAY_TYPE, CERT_NAME_xxx).
    //! @return Certificate name, empty string on error.
    //!
    TSCOREDLL UString GetCertificateName(::PCCERT_CONTEXT cert, ::DWORD type);

    //!
    //! Acquire TLS credentials.
    //! @param [out] cred Returned credentials. Must be freeed using FreeCredentialsHandle().
    //! @param [in] server True for server side, false for client side.
    //! @param [in] verify_peer Verify the certificate of the peer.
    //! @param [in] cert Optional certicate. Typically nullptr on clients.
    //! @param [in,out] report Where to report errors.
    //! @return True on success, false on error.
    //!
    TSCOREDLL bool GetCredentials(::CredHandle& cred, bool server, bool verify_peer, ::PCCERT_CONTEXT cert, Report& report);

    //!
    //! Repository of Windows certificate stores.
    //! The certificate stores must remain open all the time, once open.
    //! They are closed on termination of the singleton.
    //!
    class TSCOREDLL CertStoreRepository
    {
        TS_SINGLETON(CertStoreRepository);
    public:
        //!
        //! Destructor: close opened stores.
        //!
        ~CertStoreRepository();

        //!
        //! Get or open a certificate store.
        //! @param [in] store_name Name of certificate store. One of "system", "user".
        //! @param [in,out] report Where to report errors.
        //! @return A handle to the certificate store or nullptr on error. Do not close it, will be done by singleton destructor.
        //!
        ::HCERTSTORE getStore(const UString& store_name, Report& report);

    private:
        std::mutex _mutex {};
        std::map<UString, ::HCERTSTORE> _stores {};
    };
}
