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
#include "tsTablesDisplay.h"

namespace ts {
    //!
    //! Abstract base class for MPEG PSI/SI tables.
    //!
    class TSDUCKDLL AbstractTable
    {
    public:
        //!
        //! Check if the table is valid.
        //! @return True if the table is valid.
        //!
        bool isValid() const {return _is_valid;}

        //!
        //! Invalidate the table.
        //! This object must be rebuilt.
        //!
        void invalidate() {_is_valid = false;}

        //!
        //! Get the table_id.
        //! @return The table_id.
        //!
        TID tableId() const {return _table_id;}

        //!
        //! This abstract method serializes a table.
        //! @param [out] bin A binary table object.
        //! Its content is replaced with a binary representation of this object.
        //!
        virtual void serialize(BinaryTable& bin) const = 0;

        //!
        //! This abstract method deserializes a binary table.
        //! In case of success, this object is replaced with the interpreted content of @a bin.
        //! In case of error, this object is invalidated.
        //! @param [in] bin A binary table to interpret according to the table subclass.
        //!
        virtual void deserialize(const BinaryTable& bin) = 0;

        //!
        //! Virtual destructor
        //!
        virtual ~AbstractTable () {}

        //!
        //! Profile of a function to display a section.
        //! Each subclass should provide a staic function named @e DisplaySection
        //! which displays a section of its table-it.
        //!
        //! @param [in,out] display Display engine.
        //! @param [in] section A safe pointer to the section to display.
        //! @param [in] indent Indentation width.
        //!
        typedef void (*DisplaySectionFunction)(TablesDisplay& display, const ts::Section& section, int indent);

    protected:
        //!
        //! The table id can be modified by subclasses only
        //!
        TID _table_id;

        //!
        //! It is the responsibility of the subclasses to set the valid flag
        //!
        bool _is_valid;

        //!
        //! Protected constructor for subclasses.
        //! @param [in] tid Table id.
        //!
        AbstractTable(TID tid) : _table_id(tid), _is_valid(false) {}

        //!
        //! A utility method to dump extraneous bytes after the expected section data.
        //! Useful for static DisplaySection() methods.
        //! @param [in,out] display Display engine.
        //! @param [in] data Address of extra data to dump.
        //! @param [in] size Size of extra data to dump.
        //! @param [in] indent Indentation width.
        //!
        static void DisplayExtraData(TablesDisplay& display, const void *data, size_t size, int indent);

    private:
        // Unreachable constructors and operators.
        AbstractTable() = delete;
        AbstractTable(const AbstractTable&) = delete;
        AbstractTable& operator=(const AbstractTable&) = delete;
    };

    //!
    //! Safe pointer for AbstractTable (not thread-safe)
    //!
    typedef SafePtr <AbstractTable, NullMutex> AbstractTablePtr;
}
