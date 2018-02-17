//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsAbstractTable.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Protected constructor for subclasses.
//----------------------------------------------------------------------------

ts::AbstractTable::AbstractTable(TID tid, const UChar* xml_name) :
    AbstractSignalization(xml_name),
    _table_id(tid)
{
}


//----------------------------------------------------------------------------
// Entry base class implementation.
//----------------------------------------------------------------------------

ts::AbstractTable::EntryWithDescriptors::EntryWithDescriptors(const AbstractTable* table) :
    descs(table)
{
}

ts::AbstractTable::EntryWithDescriptors::EntryWithDescriptors(const AbstractTable* table, const EntryWithDescriptors& other) :
    descs(table, other.descs)
{
}

ts::AbstractTable::EntryWithDescriptors& ts::AbstractTable::EntryWithDescriptors::operator=(const EntryWithDescriptors& other)
{
    if (&other != this) {
        // Copying the descriptor list preserves the associated table of the target.
        descs = other.descs;
    }
    return *this;
}
