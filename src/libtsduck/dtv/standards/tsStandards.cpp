//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsStandards.h"
#include "tsNamesFile.h"
#include "tsSingleton.h"


//----------------------------------------------------------------------------
// Check compatibility between standards.
//----------------------------------------------------------------------------
//
// Compatibility matrix, one by one:
//
//           NONE  MPEG  DVB   SCTE  ATSC  ISDB JAPAN  ABNT  
//   NONE           X     X     X     X     X     X     X
//   MPEG                 X     X     X     X     X     X
//   DVB                        X     -     -     -     -
//   SCTE                             X     X     X     X
//   ATSC                                   -     -     -
//   ISDB                                         X     X
//   JAPAN                                              -
//   ABNT
//
// The following set lists all pairs of incompatible standards:
//
TS_STATIC_INSTANCE(const, std::set<ts::Standards>, IncompatibleStandards, ({
    (ts::Standards::DVB   | ts::Standards::ATSC),
    (ts::Standards::DVB   | ts::Standards::ISDB),
    (ts::Standards::DVB   | ts::Standards::JAPAN),
    (ts::Standards::DVB   | ts::Standards::ABNT),
    (ts::Standards::ATSC  | ts::Standards::ISDB),
    (ts::Standards::ATSC  | ts::Standards::JAPAN),
    (ts::Standards::ATSC  | ts::Standards::ABNT),
    (ts::Standards::JAPAN | ts::Standards::ABNT)
}));

bool ts::CompatibleStandards(Standards std)
{
    for (auto forbidden : *IncompatibleStandards) {
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
        return NameFromDTV(u"Standards", 0, NamesFlags::NAME);
    }
    else {
        UString list;
        for (Standards mask = Standards(1); mask != Standards::NONE; mask <<= 1) {
            if (bool(standards & mask)) {
                if (!list.empty()) {
                    list.append(u", ");
                }
                list.append(NameFromDTV(u"Standards", std::underlying_type<Standards>::type(mask), NamesFlags::NAME));
            }
        }
        return list;
    }
}
