 //----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDID.h"
#include "tsEDID.h"
#include "tsPSIRepository.h"


// Name of a Descriptor ID.
ts::UString ts::DIDName(DID did, DescriptorContext& context, NamesFlags flags)
{
    return Names::Format(did, PSIRepository::Instance().getDescriptor(XDID(did), context).display_name, flags, 8);
}

// Name of an MPEG extension descriptor ID.
ts::UString ts::XDIDNameMPEG(DID xdid, NamesFlags flags)
{
    return Names::Format(xdid, PSIRepository::Instance().getDescriptor(EDID::ExtensionMPEG(xdid)).display_name, flags, 8);
}

// Name of a DVB extension descriptor ID.
ts::UString ts::XDIDNameDVB(DID xdid, NamesFlags flags)
{
    return Names::Format(xdid, PSIRepository::Instance().getDescriptor(EDID::ExtensionDVB(xdid)).display_name, flags, 8);
}
