//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2025, Vision Advance Technology Inc. (VATek)
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMain.h"
#include "tsVatekControl.h"
TS_MAIN(MainCode);

int MainCode(int argc, char *argv[])
{
    ts::VatekControl opt(argc, argv);
    return opt.execute();
}
