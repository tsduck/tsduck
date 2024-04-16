//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTransportStreamId.h"
#include "tsUString.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::TransportStreamId::TransportStreamId(uint16_t tsid, uint16_t onid) :
    transport_stream_id(tsid),
    original_network_id(onid)
{
}


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
