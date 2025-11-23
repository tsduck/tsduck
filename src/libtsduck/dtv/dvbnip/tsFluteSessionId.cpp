//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFluteSessionId.h"


//----------------------------------------------------------------------------
// Explicit constructor.
//----------------------------------------------------------------------------

ts::FluteSessionId::FluteSessionId(const IPAddress& source_, const IPSocketAddress& destination_, uint64_t tsi_) :
    source(source_),
    destination(destination_),
    tsi(tsi_)
{
}


//----------------------------------------------------------------------------
// Comparison operator for use as index in maps.
//----------------------------------------------------------------------------

bool ts::FluteSessionId::operator<(const FluteSessionId& other) const
{
    if (tsi != other.tsi) {
        return tsi < other.tsi;
    }
    else if (source != other.source) {
        return source < other.source;
    }
    else {
        return destination < other.destination;
    }
}


//----------------------------------------------------------------------------
// Check if this session id "matches" another one.
//----------------------------------------------------------------------------

bool ts::FluteSessionId::match(const FluteSessionId& other) const
{
    return tsi == other.tsi && source.match(other.source) && destination.match(other.destination);
}


//----------------------------------------------------------------------------
// Implementation of StringifyInterface.
//----------------------------------------------------------------------------

ts::UString ts::FluteSessionId::toString() const
{
    return UString::Format(u"source: %s, destination: %s, TSI: %d", source, destination, tsi);
}
