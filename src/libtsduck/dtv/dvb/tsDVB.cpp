//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVB.h"
#include "tsNames.h"
#include "tsDuckContext.h"


//----------------------------------------------------------------------------
// Name of various identifiers.
//----------------------------------------------------------------------------

ts::UString ts::NetworkIdName(uint16_t id, NamesFlags flags)
{
    return NameFromSection(u"dtv", u"NetworkId", id, flags);
}

ts::UString ts::OriginalNetworkIdName(uint16_t id, NamesFlags flags)
{
    return NameFromSection(u"dtv", u"OriginalNetworkId", id, flags);
}

ts::UString ts::BouquetIdName(uint16_t id, NamesFlags flags)
{
    return NameFromSection(u"dtv", u"BouquetId", id, flags);
}

ts::UString ts::ServiceTypeName(uint8_t type, NamesFlags flags)
{
    return NameFromSection(u"dtv", u"ServiceType", type, flags);
}

ts::UString ts::DataBroadcastIdName(uint16_t id, NamesFlags flags)
{
    return NameFromSection(u"dtv", u"DataBroadcastId", id, flags);
}


//----------------------------------------------------------------------------
// Name of Running Status (in SDT).
//----------------------------------------------------------------------------

const ts::Names& ts::RunningStatusEnum()
{
    static const Names data {
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
    return NameFromSection(u"dtv", u"RunningStatus", status, flags);
}


//----------------------------------------------------------------------------
// Content ids with variants.
//----------------------------------------------------------------------------

ts::UString ts::ContentIdName(const DuckContext& duck, uint8_t x, NamesFlags flags)
{
    if (bool(duck.standards() & Standards::JAPAN)) {
        // Japan / ISDB uses a completely different mapping.
        return NameFromSection(u"dtv", u"ContentIdJapan", x, flags);
    }
    else if (bool(duck.standards() & Standards::ABNT)) {
        // ABNT (Brazil) / ISDB uses a completely different mapping.
        return NameFromSection(u"dtv", u"ContentIdABNT", x, flags);
    }
    else {
        // Standard DVB mapping.
        return NameFromSection(u"dtv", u"ContentId", x, flags);
    }
}
