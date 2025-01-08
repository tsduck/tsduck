//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsOUI.h"


//----------------------------------------------------------------------------
// Get the name of an IEEE-assigned Organizationally Unique Identifier (OUI).
//----------------------------------------------------------------------------

ts::UString ts::OUIName(uint32_t oui, NamesFlags flags)
{
    return NameFromSection(u"oui", u"OUI", oui, flags);
}
