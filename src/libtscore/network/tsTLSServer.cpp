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
// Server properties.
//----------------------------------------------------------------------------

void ts::TLSServer::setArgs(const TLSArgs& args)
{
    _certificate_store = args.certificate_store;
    _certificate_path = args.certificate_path;
    _key_path = args.key_path;
}

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
