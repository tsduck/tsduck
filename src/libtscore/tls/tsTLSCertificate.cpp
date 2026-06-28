//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------

#include "tsTLSCertificate.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::TLSCertificate::TLSCertificate(Report* report, Object* owner) :
    ReporterBase(report, owner)
{
    allocateGuts();
}

ts::TLSCertificate::TLSCertificate(ReporterBase* delegate, Object* owner) :
    ReporterBase(delegate, owner)
{
    allocateGuts();
}

ts::TLSCertificate::~TLSCertificate()
{
    if (_guts != nullptr) {
        deleteGuts();
        _guts = nullptr;
    }
}


//-----------------------------------------------------------------------------
// Initialize (get or create) a server certificate, if not already done.
//-----------------------------------------------------------------------------

bool ts::TLSCertificate::initServerCertificate(const TLSServerBase& params)
{
    if (isValid()) {
        // Get or create the certificate the first time only.
        return true;
    }
    else if (params.getEphemeralRSABits() > 0) {
        // Create an ephemeral certificate.
        return createEphemeralCertificate(params.getEphemeralRSABits());
    }
    else {
        // Fetch an existing certificate.
        return loadCertificate(params.getCertificatePath(), params.getKeyPath(), params.getCertificateStore());
    }
}
