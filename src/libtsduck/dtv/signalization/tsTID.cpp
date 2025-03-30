//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTID.h"
#include "tsDuckContext.h"
#include "tsPSIRepository.h"


// Name of a Table ID.
ts::UString ts::TIDName(const DuckContext& duck, TID tid, PID pid, CASID cas, NamesFlags flags)
{
    return Names::Format(tid, PSIRepository::Instance().getTable(tid, SectionContext(pid, duck.standards(), cas)).display_name, flags, 8);
}
