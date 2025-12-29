//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------

#include "tsTunerDevice.h"
#include "tsReport.h"


//-----------------------------------------------------------------------------
// BSD implementation of services from ts::TunerBase.
// FreeBSD, OpenBSD, NetBSD, DragonFlyBSD.
//-----------------------------------------------------------------------------

bool ts::TunerBase::GetAllTuners(DuckContext& duck, TunerPtrVector& tuners)
{
    duck.report().error(u"Digital tuners are not implemented on BSD systems");
    return false;
}
