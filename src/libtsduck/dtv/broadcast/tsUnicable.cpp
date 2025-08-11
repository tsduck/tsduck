//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard, Matthew Sweet
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsUnicable.h"


//----------------------------------------------------------------------------
// Check if the content of this object is valid and consistent.
//----------------------------------------------------------------------------

bool ts::Unicable::isValid() const
{
    return user_band_slot >= 1 &&
           ((version == 1 && user_band_slot <= 8) || (version == 2 && user_band_slot <= 32)) &&
           user_band_frequency >= 900'000'000 &&
           user_band_frequency <= 2'200'000'000;
}


//----------------------------------------------------------------------------
// Implementation of StringifyInterface
//----------------------------------------------------------------------------

ts::UString ts::Unicable::toString() const
{
    // The user band frequency is in MHz in the string.
    return UString::Format(u"%d,%d,%d", version, user_band_slot, user_band_frequency / 1'000'000);
}


//----------------------------------------------------------------------------
// Decode a string containing a Unicable representation.
//----------------------------------------------------------------------------

bool ts::Unicable::decode(const UString& str, Report& report)
{
    uint8_t vers = 0;
    uint8_t slot = 0;
    uint64_t freq = 0;
    if (!str.scan(u"%d,%d,%d", &vers, &slot, &freq)) {
        report.error(u"invalid Unicable representation: %s", str);
        return false;
    }
    if (vers < 1 || vers > 2) {
        report.error(u"invalid Unicable version %d, must be 1 or 2", vers);
        return false;
    }
    if (slot < 1 || (vers == 1 && slot > 8) || (vers == 2 && slot > 32)) {
        report.error(u"invalid Unicable user band slot %d, must be in range 1-8 (version 1) or 1-32 (version 2)", slot);
        return false;
    }
    if (freq < 900 || freq > 2200) {
        report.error(u"invalid Unicable user band frequency %d, must be in 900-2200 MHz", freq);
        return false;
    }

    // Finally got something good.
    version = vers;
    user_band_slot = slot;
    // The user band frequency is in MHz in the string.
    user_band_frequency = freq * 1'000'000;
    return true;
}


//----------------------------------------------------------------------------
// Get a string describing the format of Unicable strings.
//----------------------------------------------------------------------------

const ts::UString& ts::Unicable::StringFormat()
{
    static const UString desc =
        u"The value is of the form: <version>,<userband slot>,<userband frequency in MHz>. "
        u"Version 1 indicates EN50494 (Unicable I), version 2 indicates EN50607 (Unicable II).";
    return desc;
}


//----------------------------------------------------------------------------
// Get the default LNB for Unicable switches.
//----------------------------------------------------------------------------

bool ts::Unicable::GetDefaultLNB(LNB& lnb, Report& report)
{
    // Unicable switches assume a "European Universal Ku (extended)" LNB.
    static const UChar lnb_name[] = u"Extended";

    if (lnb.set(lnb_name, report)) {
        report.debug(u"loaded LNB \"%s\" for Unicable", lnb);
        return true;
    }
    else {
        report.error(u"LNB \"%s\" not found for Unicable", lnb_name);
        return false;
    }
}
