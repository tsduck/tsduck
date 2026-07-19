//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsReactiveInputControl.h"


void ts::ReactiveInputControl::reset()
{
    used_size.reset();
    min_next_size.reset();
    next_delimiter.reset();
}
