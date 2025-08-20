//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  SSL/TLS Server
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTCPServer.h"
#include "tsTLSArgs.h"
#include "tsTLSConnection.h"

namespace ts {
    //!
    //! Implementation of a SSL/TLS server.
    //! @ingroup libtscore net
    //!
    //! Creating a test private key and its self-signed certificate
    //! -----------------------------------------------------------
    //! On UNIX systems (with OpenSSL):
    //! @code
    //! openssl req -quiet -newkey rsa:3072 -new -noenc -x509 -subj="/CN=$(hostname)" -days 3650 -keyout key.pem -out cert.pem
    //! @endcode
    //!
    //! To display the properties of the certificate:
    //! @code
    //! openssl x509 -in cert.pem -noout -text
    //! @endcode
    //!
    //! On Windows:
    //! @code
    //! New-SelfSignedCertificate -FriendlyName "Test Server" -Type SSLServerAuthentication `
    //!     -DnsName @([System.Net.Dns]::GetHostName(), "localhost") `
    //!     -CertStoreLocation Cert:\CurrentUser\My `
    //!     -KeyAlgorithm "RSA" -KeyLength 3072
    //! @endcode
    //!
    //! To display the properties of the certificate:
    //! @code
    //! Get-ChildItem Cert:\CurrentUser\My | Where-Object -Property FriendlyName -eq "Test Server" | Format-List
    //! @endcode
    //!
    //! A self-signed certificate is considered as invalid. So, make sure to ignore
    //! this error. With curl, use option --insecure or -k.
    //!
    //! To view the certificate of a server using OpenSSL:
    //! @code
    //! openssl s_client -showcerts -servername <name> -connect <name>:<port> </dev/null | openssl x509 -noout -text
    //! @endcode
    //!
    class TSCOREDLL TLSServer: public TCPServer
    {
        TS_NOCOPY(TLSServer);
    public:
        //!
        //! Reference to the superclass.
        //!
        using SuperClass = TCPServer;

        //!
        //! Constructor.
        //!
        TLSServer();

        //!
        //! Constructor with initial arguments.
        //! @param [in] args Initial TLS arguments.
        //!
        TLSServer(const TLSArgs& args) : TLSServer() { setArgs(args); }

        //!
        //! Set command line arguments for the server.
        //! @param [in] args TLS arguments.
        //!
        void setArgs(const TLSArgs& args);

        //!
        //! Set the certificate path for the server.
        //! @param [in] path Path to the certificate.
        //! - On UNIX systems (with OpenSSL), this is the path name of the certificate file in PEM format.
        //! - On Windows, this is the name of a certificate, either its "friendly name", its subject name (without "CN="), its DNS name.
        //!
        void setCertificatePath(const UString& path) { _certificate_path = path; }

        //!
        //! Get the certificate path for the server.
        //! @return A constant reference to the path to the certificate.
        //! @see setCertificatePath()
        //!
        const UString& getCertificatePath() const { return _certificate_path; }

        //!
        //! Set the private key path for the server.
        //! @param [in] path Path to the private key.
        //! - On UNIX systems (with OpenSSL), this is the path name of the private key file in PEM format.
        //! - On Windows, the private key is retrieved with the certificate and this parameter is unused.
        //!
        void setKeyPath(const UString& path) { _key_path = path; }

        //!
        //! Get the private key path for the server.
        //! @return A constant reference to the path to the private key.
        //! @see setKeyPath()
        //!
        const UString& getKeyPath() const { return _key_path; }

        //!
        //! Set the certificate store.
        //! @param [in] name
        //! - On UNIX systems (with OpenSSL), this parameter is unused.
        //! - On Windows, the possible values are "system" (<code>Cert:\\LocalMachine\\My</code>)
        //!   and "user" (<code>Cert:\\CurrentUser\\My</code>). The default is "user".
        //!
        void setCertificateStore(const UString& name) { _certificate_store = name; }

        //!
        //! Get the certificate store.
        //! @return A constant reference to the name of the certificate store.
        //! @see setCertificateStore()
        //!
        const UString& getCertificateStore() const { return _certificate_store; }

        // Inherited methods.
        virtual ~TLSServer() override;
        virtual bool listen(int backlog, Report& report = CERR) override;
        virtual bool accept(TCPConnection& client, IPSocketAddress& addr, Report& report = CERR) override;
        virtual bool close(Report& report = CERR) override;

    private:
        // System-specific parts are stored in a private structure.
        // This is done to avoid inclusion of specialized headers in this public file.
        class SystemGuts;
        SystemGuts* _guts = nullptr;

        // Common properties.
        UString _certificate_store {};
        UString _certificate_path {};
        UString _key_path {};

        // Allocate and deallocate guts (depend on implementations).
        void allocateGuts();
        void deleteGuts();

        // Version of accept() with an explicit TLSConnection.
        bool acceptTLS(TLSConnection& client, IPSocketAddress& addr, Report& report = CERR);
    };
}
