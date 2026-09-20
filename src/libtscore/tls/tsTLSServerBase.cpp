//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTLSServerBase.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::TLSServerBase::~TLSServerBase()
{
}


//----------------------------------------------------------------------------
// Get the size in bits of the ephemeral RSA key.
//----------------------------------------------------------------------------

size_t ts::TLSServerBase::getEphemeralRSABits() const
{
    if (_tls_args.ephemeral_rsa_bits > 0) {
        // Explicit key size.
        return _tls_args.ephemeral_rsa_bits;
    }
    else if (_tls_args.hasCertificate()) {
        // A certificate is specified, do not use any ephemeral RSA key.
        return 0;
    }
    else {
        // Nothing is specified, use an ephemeral RSA key by default.
        return DEFAULT_RSA_BITS;
    }
}
