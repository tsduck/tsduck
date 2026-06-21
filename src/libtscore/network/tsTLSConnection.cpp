//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTLSConnection.h"
#include "tsFeatures.h"


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

ts::TLSConnection::TLSConnection(Report* report, Object* owner) :
    TCPConnection(report, false, owner)
{
    allocateGuts();
}

ts::TLSConnection::TLSConnection(ReporterBase* delegate, Object* owner) :
    TCPConnection(delegate, false, owner)
{
    allocateGuts();
}

ts::TLSConnection::~TLSConnection()
{
    if (_guts != nullptr) {
        deleteGuts();
        _guts = nullptr;
    }
}


//----------------------------------------------------------------------------
// TLSConnection must use blocking I/O.
//----------------------------------------------------------------------------

bool ts::TLSConnection::checkBlocking()
{
    if (isNonBlocking()) {
        report().error(u"internal error: TLSConnection called in non-blocking mode, use ReactiveTLSConnection");
        return false;
    }
    else {
        return true;
    }
}


//----------------------------------------------------------------------------
// Receive data until buffer is full.
//----------------------------------------------------------------------------

bool ts::TLSConnection::receive(void* buffer, size_t size, const AbortInterface* abort)
{
    // The superclass implements its fixed-length method using the variable-length method.
    return TCPConnection::receive(buffer, size, abort);
}
