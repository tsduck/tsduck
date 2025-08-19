//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTLSConnection.h"
#include "tsFeatures.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Register for options --version and --support.
//----------------------------------------------------------------------------

#if !defined(TS_WINDOWS) && defined(TS_NO_OPENSSL)
    #define SUPPORT ts::Features::UNSUPPORTED
#else
    #define SUPPORT ts::Features::SUPPORTED
#endif

TS_REGISTER_FEATURE(u"tls", u"TLS library", SUPPORT, ts::TLSConnection::GetLibraryVersion);

// A symbol to reference to force the TLS feature in static link.
const int ts::TLSConnection::FEATURE = 0;


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::TLSConnection::TLSConnection()
{
    allocateGuts();
    CheckNonNull(_guts);
}

ts::TLSConnection::~TLSConnection()
{
    if (_guts != nullptr) {
        deleteGuts();
        _guts = nullptr;
    }
}


//----------------------------------------------------------------------------
// Set command line arguments for the client.
//----------------------------------------------------------------------------

void ts::TLSConnection::setArgs(const TLSArgs& args)
{
    setServerName(args.server_name);
    _verify_peer = !args.insecure;
}


//----------------------------------------------------------------------------
// For a client connection, specify the server names.
//----------------------------------------------------------------------------

void ts::TLSConnection::setServerName(const UString& server_name)
{
    _server_name = server_name;
    _additional_names.clear();
    IPSocketAddress::RemovePort(_server_name);
}

void ts::TLSConnection::addVerifyServer(const UString& name)
{
    _additional_names.push_back(name);
}


//----------------------------------------------------------------------------
// Receive data until buffer is full.
//----------------------------------------------------------------------------

bool ts::TLSConnection::receive(void* buffer, size_t size, const AbortInterface* abort, Report& report)
{
    // The superclass implements its fixed-length method using the variable-length method.
    return SuperClass::receive(buffer, size, abort, report);
}
