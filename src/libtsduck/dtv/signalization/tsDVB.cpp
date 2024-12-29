//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDVB.h"


// Enumeration description of running status values.
const ts::Enumeration& ts::RunningStatusEnum()
{
    static const Enumeration data {
        {u"undefined",   RS_UNDEFINED},
        {u"not-running", RS_NOT_RUNNING},
        {u"starting",    RS_STARTING},
        {u"pausing",     RS_PAUSING},
        {u"running",     RS_RUNNING},
        {u"off-air",     RS_OFF_AIR},
    };
    return data;
}
