//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsServiceIdTriplet.h"
#include "tsUString.h"


//----------------------------------------------------------------------------
// Clear the content of this object.
//----------------------------------------------------------------------------

void ts::ServiceIdTriplet::clear()
{
    transport_stream_id = original_network_id = service_id = 0;
    version = 0;
}


//----------------------------------------------------------------------------
// Implementation of StringifyInterface.
//----------------------------------------------------------------------------

ts::UString ts::ServiceIdTriplet::toString() const
{
    UString str;
    str.format(u"service: 0x%X (%<d), TS: 0x%X (%<d), network: 0x%X (%<d)", {service_id, transport_stream_id, original_network_id});
    if (version != 0) {
        // Default version 0 does not matter in most cases.
        str.format(u", version %d", {version});
    }
    return str;
}
