//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSCTE52.h"

ts::UString ts::SCTE52_2003::name() const
{
    return u"ANSI/SCTE 52 (2003)";
}

ts::UString ts::SCTE52_2008::name() const
{
    return u"ANSI/SCTE 52 (2008)";
}

bool ts::SCTE52_2003::setShortIV(const void* iv_, size_t iv_length)
{
    // In the 2003 version, the IV are identical, there is no specific IV for short blocks.
    // Override this one to make it private.
    return DVS042<DES>::setShortIV(iv_, iv_length);
}
