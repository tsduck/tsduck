//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
#include "tsTLSServerBase.h"
#include "tsTLSConnection.h"
#include "tsTLSCertificate.h"

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
    class TSCOREDLL TLSServer: public TCPServer, public TLSServerBase
    {
        TS_NOCOPY(TLSServer);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit TLSServer(Report* report = nullptr, Object* owner = nullptr);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit TLSServer(ReporterBase* delegate, Object* owner = nullptr);

        //!
        //! Constructor with initial arguments.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] args Initial TLS arguments.
        //!
        TLSServer(Report* report, const TLSArgs& args) : TLSServer(report) { setArgs(args); }

        //!
        //! Constructor with initial arguments.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] args Initial TLS arguments.
        //!
        TLSServer(ReporterBase* delegate, const TLSArgs& args) : TLSServer(delegate) { setArgs(args); }

        // Inherited methods.
        virtual ~TLSServer() override;
        virtual bool listen(int backlog) override;
        virtual bool accept(TCPConnection& client, IPSocketAddress& addr, IOSB* = nullptr) override;

    protected:
        // Inherited methods.
        virtual bool closeImplementation(bool silent) override;

    private:
        TLSCertificate _cert {this, this};
    };
}
