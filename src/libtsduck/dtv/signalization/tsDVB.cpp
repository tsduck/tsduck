//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVB.h"
#include "tsDuckContext.h"


//----------------------------------------------------------------------------
// Name of various identifiers.
//----------------------------------------------------------------------------

ts::UString ts::NetworkIdName(uint16_t id, NamesFlags flags)
{
    return NameFromDTV(u"NetworkId", NamesFile::Value(id), flags);
}

ts::UString ts::OriginalNetworkIdName(uint16_t id, NamesFlags flags)
{
    return NameFromDTV(u"OriginalNetworkId", NamesFile::Value(id), flags);
}

ts::UString ts::BouquetIdName(uint16_t id, NamesFlags flags)
{
    return NameFromDTV(u"BouquetId", NamesFile::Value(id), flags);
}

ts::UString ts::ServiceTypeName(uint8_t type, NamesFlags flags)
{
    return NameFromDTV(u"ServiceType", NamesFile::Value(type), flags);
}

ts::UString ts::DataBroadcastIdName(uint16_t id, NamesFlags flags)
{
    return NameFromDTV(u"DataBroadcastId", NamesFile::Value(id), flags);
}


//----------------------------------------------------------------------------
// Name of Running Status (in SDT).
//----------------------------------------------------------------------------

const ts::Enumeration& ts::RunningStatusEnum()
{
    static const Enumeration data {
        {u"undefined",   RS_UNDEFINED},
        {u"not-running", RS_NOT_RUNNING},
        {u"starting",    RS_STARTING},
        {u"pausing",     RS_PAUSING},
        {u"running",     RS_RUNNING},
        {u"off-air",     RS_OFF_AIR},
    };
    return data;
}

ts::UString ts::RunningStatusName(uint8_t status, NamesFlags flags)
{
    return NameFromDTV(u"RunningStatus", NamesFile::Value(status), flags);
}


//----------------------------------------------------------------------------
// Content ids with variants.
//----------------------------------------------------------------------------

ts::UString ts::ContentIdName(const DuckContext& duck, uint8_t x, NamesFlags flags)
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
