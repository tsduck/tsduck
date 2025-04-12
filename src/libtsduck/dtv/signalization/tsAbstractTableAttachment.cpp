//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractTableAttachment.h"
#include "tsAbstractTable.h"


ts::TID ts::AbstractTableAttachment::tableId() const
{
    return _table == nullptr ? TID(TID_NULL) : _table->tableId();
}

ts::Standards ts::AbstractTableAttachment::tableStandards() const
{
    return _table == nullptr ? Standards::NONE : _table->definingStandards();
}
