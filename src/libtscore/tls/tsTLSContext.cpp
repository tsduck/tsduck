//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------

#include "tsTLSContext.h"
#include "tsFeatures.h"


//----------------------------------------------------------------------------
// Register for options --version and --support.
//----------------------------------------------------------------------------

#if !defined(TS_WINDOWS) && defined(TS_NO_OPENSSL)
    #define SUPPORT ts::Features::UNSUPPORTED
#else
    #define SUPPORT ts::Features::SUPPORTED
#endif

TS_REGISTER_FEATURE(u"tls", u"TLS library", SUPPORT, ts::TLSContext::GetLibraryVersion);


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::TLSContext::TLSContext(Report* report) :
    ReporterBase(report)
{
    allocateGuts();
}

ts::TLSContext::TLSContext(ReporterBase* delegate) :
    ReporterBase(delegate)
{
    allocateGuts();
}

ts::TLSContext::~TLSContext()
{
    if (_guts != nullptr) {
        deleteGuts();
        _guts = nullptr;
    }
}
