//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTLSServer.h"
#include "tsEnvironment.h"
#include "tsArgs.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::TLSServer::TLSServer()
{
    allocateGuts();
    CheckNonNull(_guts);
}

ts::TLSServer::~TLSServer()
{
    if (_guts != nullptr) {
        deleteGuts();
        _guts = nullptr;
    }
}


//----------------------------------------------------------------------------
// Get server properties.
//----------------------------------------------------------------------------

ts::UString ts::TLSServer::getCertificatePath() const
{
    return _certificate_path.empty() ? GetEnvironment(u"TSDUCK_TLS_CERTIFICATE") : _certificate_path;
}

ts::UString ts::TLSServer::getKeyPath() const
{
    return _key_path.empty() ? GetEnvironment(u"TSDUCK_TLS_KEY") : _key_path;
}

ts::UString ts::TLSServer::getCertificateStore() const
{
    return _certificate_store.empty() ? GetEnvironment(u"TSDUCK_TLS_STORE", u"user") : _certificate_store;
}


//----------------------------------------------------------------------------
// Wait for a client (inherited version).
//----------------------------------------------------------------------------

bool ts::TLSServer::accept(TCPConnection& client, IPSocketAddress& addr, Report& report)
{
    TLSConnection* tls = dynamic_cast<TLSConnection*>(&client);
    if (tls != nullptr) {
        return acceptTLS(*tls, addr, report);
    }
    else {
        report.error(u"internal programming error: TLSServer::accept() needs a TLSConnection");
        return false;
    }
}


//----------------------------------------------------------------------------
// Add command line options for a TLS server in an Args.
//----------------------------------------------------------------------------

void ts::TLSServer::DefineArgs(Args& args)
{
    args.option(u"certificate-path", 0, Args::STRING);
    args.help(u"certificate-path", u"name",
              u"Path to the certificate for the TLS server.\n"
              u"On UNIX systems, this is the path name of the certificate file in PEM format.\n"
              u"On Windows, this is the name of a certificate.");

    args.option(u"key-path", 0, Args::STRING);
    args.help(u"key-path", u"name",
              u"Path to the private key for the TLS server.\n"
              u"On UNIX systems, this is the path name of the private key file in PEM format.\n"
              u"On Windows, the private key is retrieved with the certificate and this parameter is unused.");

    args.option(u"store", 0, Args::STRING);
    args.help(u"store", u"name",
              u"Specify the name of the certificate store for the TLS server.\n"
              u"On Windows, the possible values are \"system\" (Cert:\\LocalMachine\\My) "
              u"or \"user\" (Cert:\\CurrentUser\\My). The default is \"user\".\n"
              u"On UNIX systems, this parameter is unused.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::TLSServer::loadArgs(Args& args)
{
    args.getValue(_certificate_path, u"certificate-path");
    args.getValue(_key_path, u"key-path");
    args.getValue(_certificate_store, u"store");
    return true;
}
