//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsSCTE52.h"

TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::SCTE52_2003, SCTE52, (DVS042<DES>::PROPERTIES(), u"ANSI/SCTE 52 (2003)", nullptr, 0));
TS_BLOCK_CIPHER_DEFINE_PROPERTIES(ts::SCTE52_2008, SCTE52, (DVS042<DES>::PROPERTIES(), u"ANSI/SCTE 52 (2008)", nullptr, 0));

ts::SCTE52_2003::SCTE52_2003() : DVS042<DES>(SCTE52_2003::PROPERTIES(), true)
{
}

ts::SCTE52_2003::~SCTE52_2003()
{
}

ts::SCTE52_2008::SCTE52_2008() : DVS042<DES>(SCTE52_2008::PROPERTIES())
{
}

ts::SCTE52_2008::~SCTE52_2008()
{
}
