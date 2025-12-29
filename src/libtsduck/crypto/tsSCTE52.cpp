//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSCTE52.h"

const ts::BlockCipherProperties& ts::SCTE52_2003::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(DVS042<DES>::Properties(), u"ANSI/SCTE 52 (2003)", nullptr, 0);
    return props;
}

ts::SCTE52_2003::SCTE52_2003() : DVS042<DES>(SCTE52_2003::Properties(), true)
{
}

ts::SCTE52_2003::~SCTE52_2003()
{
}

const ts::BlockCipherProperties& ts::SCTE52_2008::Properties()
{
    // Thread-safe init-safe static data pattern:
    static const BlockCipherProperties props(DVS042<DES>::Properties(), u"ANSI/SCTE 52 (2008)", nullptr, 0);
    return props;
}

ts::SCTE52_2008::SCTE52_2008() : DVS042<DES>(SCTE52_2008::Properties())
{
}

ts::SCTE52_2008::~SCTE52_2008()
{
}
