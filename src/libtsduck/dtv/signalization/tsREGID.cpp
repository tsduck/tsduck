//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsREGID.h"


//----------------------------------------------------------------------------
// Name of a Registration id from an MPEG registration_descriptor.
//----------------------------------------------------------------------------

ts::UString ts::REGIDName(REGID regid, NamesFlags flags)
{
    // If a name exists for the value, use it.
    UString name(NameFromSection(u"dtv", u"mpeg.registration_id", regid, flags | NamesFlags::NO_UNKNOWN));
    if (!name.empty()) {
        return name;
    }

    // Registration ids are often 32-bits ASCII string. Check if this is the case.
    name = u"\"";
    for (int i = 24; i >= 0; i -= 8) {
        const uint8_t c = uint8_t(regid >> i);
        if (c >= 0x20 && c <= 0x7E) {
            // This is an ASCII character.
            name.push_back(c);
        }
        else {
            // This is not a full-ASCII string.
            name.clear();
            break;
        }
    }
    if (!name.empty()) {
        name.push_back(u'"');
    }
    return Names::Format(regid, name, flags, 32);
}
