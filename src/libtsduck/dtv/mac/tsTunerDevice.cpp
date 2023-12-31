//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------

#include "tsTunerDevice.h"
#include "tsReport.h"


//-----------------------------------------------------------------------------
// MacOS implementation of services from ts::TunerBase.
//-----------------------------------------------------------------------------

bool ts::TunerBase::GetAllTuners(DuckContext& duck, TunerPtrVector& tuners)
{
    duck.report().error(u"Digital tuners are not implemented on macOS");
    return false;
}
