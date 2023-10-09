//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsStandards.h"
#include "tsNamesFile.h"


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
