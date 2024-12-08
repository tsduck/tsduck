//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsEDID.h"


// Build the EDID for an MPEG or DVB extension descriptor.
ts::EDID ts::EDID::Extension(XDID xdid)
{
    return xdid.isExtensionMPEG() ? ExtensionMPEG(xdid.edid()) : (xdid.isExtensionDVB() ? ExtensionDVB(xdid.edid()) : Regular(xdid.did(), Standards::NONE));
}


// Check if the descriptor is table-specific and matches a given table id.
// If standards are specified in the context and in the EDID, they must have at least one in common.
bool ts::EDID::matchTableSpecific(TID tid, Standards std) const
{
    return tid != TID_NULL && tableId() == tid && (!std || !standards() || bool(std & standards()));
}


// Check if the descriptor is a regular one and matches at least one standard.
// If the regular descriptor has declared no standard, then it matches by default.
bool ts::EDID::matchRegularStandards(Standards std) const
{
    return isRegular() && (!std || !standards() || bool(std & standards()));
}


// Build an eXtension Descriptor Id from the EDID.
ts::XDID ts::EDID::xdid() const
{
    return XDID(DID(_edid & 0xFF), DID(type() == Type::EXTENDED ? ((_edid >> 8) & 0xFF) : EDID_NULL));
}
