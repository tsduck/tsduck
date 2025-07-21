//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
// SSL/TLS connection - Windows specific parts with SChannel.
//
//----------------------------------------------------------------------------

#include "tsTLSConnection.h"
#include "tsWinTLS.h"


//----------------------------------------------------------------------------
// Library version.
//----------------------------------------------------------------------------

ts::UString ts::TLSConnection::GetLibraryVersion()
{
    // Don't know how to get the version of SChannel library.
    return u"Microsoft SChannel";
}


//----------------------------------------------------------------------------
// System-specific parts are stored in a private structure.
//----------------------------------------------------------------------------

class ts::TLSConnection::SystemGuts
{
    TS_NOCOPY(SystemGuts);
public:
    // Constructor and destructor.
    SystemGuts() = default;
    ~SystemGuts();
};


//----------------------------------------------------------------------------
// System-specific constructor and destructor.
//----------------------------------------------------------------------------

void ts::TLSConnection::allocateGuts()
{
    _guts = new SystemGuts;
}

void ts::TLSConnection::deleteGuts()
{
    delete _guts;
}

ts::TLSConnection::SystemGuts::~SystemGuts()
{
}


//----------------------------------------------------------------------------
// Connect to a remote address and port.
//----------------------------------------------------------------------------

bool ts::TLSConnection::connect(const IPSocketAddress&, Report& report)
{
    // Acquire credentials.
    ::CredHandle cred;
    if (!GetCredentials(cred, false, _verify_server, nullptr, report)) {
        return false;
    }

    //@@@@ TO BE CONTINUED
    ::FreeCredentialsHandle(&cred);

    report.error(u"not yet implemented");
    return false;
}


//----------------------------------------------------------------------------
// Close the write direction of the connection.
//----------------------------------------------------------------------------

bool ts::TLSConnection::closeWriter(Report& report)
{
    //@@@@ TO BE CONTINUED
    report.error(u"not yet implemented");
    return false;
}


//----------------------------------------------------------------------------
// Disconnect from remote partner.
//----------------------------------------------------------------------------

bool ts::TLSConnection::disconnect(Report& report)
{
    //@@@@ TO BE CONTINUED
    report.error(u"not yet implemented");
    return false;
}


//----------------------------------------------------------------------------
// Send data.
//----------------------------------------------------------------------------

bool ts::TLSConnection::send(const void* data, size_t size, Report& report)
{
    //@@@@ TO BE CONTINUED
    report.error(u"not yet implemented");
    return false;
}


//----------------------------------------------------------------------------
// Receive data.
//----------------------------------------------------------------------------

bool ts::TLSConnection::receive(void* buffer, size_t max_size, size_t& ret_size, const AbortInterface* abort, Report& report)
{
    //@@@@ TO BE CONTINUED
    report.error(u"not yet implemented");
    return false;
}
