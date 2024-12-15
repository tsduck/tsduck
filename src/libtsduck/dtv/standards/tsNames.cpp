//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsNames.h"
#include "tsTID.h"
#include "tsCAS.h"
#include "tsDuckContext.h"


//----------------------------------------------------------------------------
// Public functions returning names.
//----------------------------------------------------------------------------

ts::UString ts::names::BouquetId(uint16_t id, NamesFlags flags)
{
    return NameFromDTV(u"BouquetId", NamesFile::Value(id), flags);
}

ts::UString ts::names::OriginalNetworkId(uint16_t id, NamesFlags flags)
{
    return NameFromDTV(u"OriginalNetworkId", NamesFile::Value(id), flags);
}

ts::UString ts::names::NetworkId(uint16_t id, NamesFlags flags)
{
    return NameFromDTV(u"NetworkId", NamesFile::Value(id), flags);
}

ts::UString ts::names::DataBroadcastId(uint16_t id, NamesFlags flags)
{
    return NameFromDTV(u"DataBroadcastId", NamesFile::Value(id), flags);
}

ts::UString ts::names::ServiceType(uint8_t type, NamesFlags flags)
{
    return NameFromDTV(u"ServiceType", NamesFile::Value(type), flags);
}

ts::UString ts::names::RunningStatus(uint8_t status, NamesFlags flags)
{
    return NameFromDTV(u"RunningStatus", NamesFile::Value(status), flags);
}


//----------------------------------------------------------------------------
// Content ids with variants.
//----------------------------------------------------------------------------

ts::UString ts::names::Content(const DuckContext& duck, uint8_t x, NamesFlags flags)
{
    if (bool(duck.standards() & Standards::JAPAN)) {
        // Japan / ISDB uses a completely different mapping.
        return NameFromDTV(u"ContentIdJapan", NamesFile::Value(x), flags);
    }
    else if (bool(duck.standards() & Standards::ABNT)) {
        // ABNT (Brazil) / ISDB uses a completely different mapping.
        return NameFromDTV(u"ContentIdABNT", NamesFile::Value(x), flags);
    }
    else {
        // Standard DVB mapping.
        return NameFromDTV(u"ContentId", NamesFile::Value(x), flags);
    }
}
