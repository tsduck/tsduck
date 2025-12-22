//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastNIPService.h"


//----------------------------------------------------------------------------
// Clear the content of the structure.
//----------------------------------------------------------------------------

void ts::mcast::NIPService::clear()
{
    channel_number = 0;
    selectable = true;
    visible = true;
    service_name.clear();
    service_type.clear();
    provider_name.clear();
    instances.clear();
}
