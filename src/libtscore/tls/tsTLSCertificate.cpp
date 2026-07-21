//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------

#include "tsTLSCertificate.h"
#include "tsSysInfo.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::TLSCertificate::TLSCertificate(Report* report) :
    ReporterBase(report)
{
    allocateGuts();
}

ts::TLSCertificate::TLSCertificate(ReporterBase* delegate) :
    ReporterBase(delegate)
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


//-----------------------------------------------------------------------------
// Get the certificate subject (CN), built from the hostname.
//-----------------------------------------------------------------------------

ts::UString ts::TLSCertificate::Subject()
{
    // The CN field is limited to 64 characters and this function returns the 64 right-most characters of the hostname.
    static constexpr size_t max_size = 64;
    const UString name(SysInfo::Instance().hostName());
    return name.size() <= max_size ? name : name.substr(name.size() - max_size);
}
