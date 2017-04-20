//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!
//!  @file
//!  Abstract base class for MPEG PSI/SI tables
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsBinaryTable.h"

namespace ts {

    class TSDUCKDLL AbstractTable
    {
    public:
        // Check if the table is valid
        bool isValid() const {return _is_valid;}

        // Invalidate the table. Must be rebuilt.
        void invalidate() {_is_valid = false;}

        // Get the table_id
        TID tableId() const {return _table_id;}

        // This abstract method serializes a table.
        // The content of the BinaryTable is replaced with a binary
        // representation (a list of sections) of this object.
        virtual void serialize (BinaryTable& table) const = 0;

        // This abstract method deserializes a binary table.
        // This object represents the interpretation of the binary table.
        virtual void deserialize (const BinaryTable& table) = 0;

        // Virtual destructor
        virtual ~AbstractTable () {}

    protected:
        // The table_id can be modified by subclasses only
        TID _table_id;

        // It is the responsibility of the subclasses to set the valid flag
        bool _is_valid;

        // Protected constructor for subclasses
        AbstractTable (TID tid) : _table_id (tid), _is_valid (false) {}

    private:
        // Unreachable constructors and operators.
        AbstractTable ();
        AbstractTable (const AbstractTable&);
        AbstractTable& operator= (const AbstractTable&);
    };

    // Safe pointer for AbstractTable (not thread-safe)
    typedef SafePtr <AbstractTable, NullMutex> AbstractTablePtr;
}
