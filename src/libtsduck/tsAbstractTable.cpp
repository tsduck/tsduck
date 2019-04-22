//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
#include "tsBinaryTable.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::AbstractTable::AbstractTable(TID tid, const UChar* xml_name, Standards standards) :
    AbstractSignalization(xml_name, standards),
    _table_id(tid)
{
}

ts::AbstractTable::~AbstractTable()
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


//----------------------------------------------------------------------------
// This method checks if a table id is valid for this object.
//----------------------------------------------------------------------------

bool ts::AbstractTable::isValidTableId(TID tid) const
{
    // The default implementation checks that the TID is identical to the TID of this object.
    return tid == _table_id;
}


//----------------------------------------------------------------------------
// This method serializes a table.
//----------------------------------------------------------------------------

void ts::AbstractTable::serialize(DuckContext& duck, BinaryTable& table) const
{
    // Reinitialize table object.
    table.clear();

    // Return an empty table if this object is not valid.
    if (!_is_valid) {
        return;
    }

    // Call the subclass implementation.
    serializeContent(duck, table);

    // Add the standards of the serialized table into the context.
    duck.addStandards(definingStandards());
}


//----------------------------------------------------------------------------
// This method deserializes a binary table.
//----------------------------------------------------------------------------

void ts::AbstractTable::deserialize(DuckContext& duck, const BinaryTable& table)
{
    // Invalidate this object.
    // Note that deserializeContent() is still responsible for clearing specific fields.
    _is_valid = false;

    // Keep this object invalid if the binary table is invalid or has an incorrect table if for this class.
    if (!table.isValid() || !isValidTableId(table.tableId())) {
        return;
    }

    // Table is already checked to be compatible but can be different from current one.
    // So, we need to update this object.
    _table_id = table.tableId();

    // Call the subclass implementation.
    deserializeContent(duck, table);

    // Add the standards of the deserialized table into the context.
    duck.addStandards(definingStandards());
}
