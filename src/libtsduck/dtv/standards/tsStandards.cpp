//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsStandards.h"
#include "tsNames.h"


//----------------------------------------------------------------------------
// Check compatibility between standards.
//----------------------------------------------------------------------------

bool ts::CompatibleStandards(Standards std)
{
    //
    // Compatibility matrix, one by one:
    //
    //           NONE  MPEG  DVB   SCTE  ATSC  ISDB JAPAN  ABNT
    //   NONE           X     X     X     X     X     X     X
    //   MPEG                 X     X     X     X     X     X
    //   DVB                        X     -    (X)   (X)   (X)
    //   SCTE                             X     X     X     X
    //   ATSC                                   -     -     -
    //   ISDB                                         X     X
    //   JAPAN                                              -
    //   ABNT
    //
    //  X  : Compatible.
    // (X) : Mixed compatibility. ISDB is based on a subset of DVB and adds other
    //       tables and descriptors. The DVB subset is compatible with ISDB. When
    //       another DID or TID is defined with two distinct semantics, one for DVB
    //       and one for ISDB, if ISDB is part of the current standards we use the
    //       ISDB semantics, otherwise we use the DVB semantics. This mixed
    //       compatibility is disabled by DVBONLY.
    //
    // The following set lists all pairs of incompatible standards
    // (thread-safe init-safe static data patterns).
    //
    static const std::set<Standards> incompatible_standards {
        (Standards::ATSC    | Standards::DVB),
        (Standards::ATSC    | Standards::ISDB),
        (Standards::ATSC    | Standards::JAPAN),
        (Standards::ATSC    | Standards::ABNT),
        (Standards::JAPAN   | Standards::ABNT),
        (Standards::DVBONLY | Standards::ISDB),
        (Standards::DVBONLY | Standards::JAPAN),
        (Standards::DVBONLY | Standards::ABNT)
    };

    for (auto forbidden : incompatible_standards) {
        if ((std & forbidden) == forbidden) {
            return false; // contains a pair of incompatible standards
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Return a string representing a list of standards.
//----------------------------------------------------------------------------

ts::UString ts::StandardsNames(Standards standards)
{
    if (standards == Standards::NONE) {
        return NameFromSection(u"dtv", u"Standards", 0, NamesFlags::NAME);
    }
    else {
        UString list;
        for (Standards mask = Standards(1); mask != Standards::NONE; mask <<= 1) {
            // DVBONLY is a marker, not a standard, don't display it.
            if (mask != Standards::DVBONLY && bool(standards & mask)) {
                if (!list.empty()) {
                    list.append(u", ");
                }
                list.append(NameFromSection(u"dtv", u"Standards", std::underlying_type<Standards>::type(mask)));
            }
        }
        return list;
    }
}
