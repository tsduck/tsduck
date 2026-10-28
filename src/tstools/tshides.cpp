//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsHiDesControl.h"
TS_MAIN(MainCode);

int MainCode(int argc, char *argv[])
{
    ts::HiDesControl opt(argc, argv);
    return opt.execute();
}
