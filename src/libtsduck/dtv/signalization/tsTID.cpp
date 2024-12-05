//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTID.h"
#include "tsDuckContext.h"
#include "tsPSIRepository.h"


// Name of a Table ID.
ts::UString ts::TIDName(const DuckContext& duck, TID tid, CASID cas, NamesFlags flags)
{
    return NamesFile::Formatted(tid, PSIRepository::Instance().getTable(tid, SectionContext(PID_NULL, duck.standards(), cas)).display_name, flags, 8);
}
