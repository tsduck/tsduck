//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTransportStreamId.h"
#include "tsUString.h"


//----------------------------------------------------------------------------
// Clear the content of this object.
//----------------------------------------------------------------------------

void ts::TransportStreamId::clear()
{
    transport_stream_id = original_network_id = 0;
}


//----------------------------------------------------------------------------
// Implementation of StringifyInterface.
//----------------------------------------------------------------------------

ts::UString ts::TransportStreamId::toString() const
{
    return UString::Format(u"TS: %n, O.Net:%n", transport_stream_id, original_network_id);
}
