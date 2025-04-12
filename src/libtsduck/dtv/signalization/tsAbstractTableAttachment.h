//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  List of MPEG PSI/SI descriptors
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTID.h"
#include "tsStandards.h"

namespace ts {

    class AbstractTable;

    //!
    //! Base class for objects which are attached to an AbstractTable.
    //! An instance of such object classes is permanently attached to a table.
    //! The link to the table is established in the constructor and never changes.
    //! Assigning instances means copy to object contents but leave the table
    //! attachment of the target unchanged.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL AbstractTableAttachment
    {
        TS_NO_DEFAULT_CONSTRUCTORS(AbstractTableAttachment);
    public:
        //!
        //! Basic constructor.
        //! @param [in] table Parent table. An instance is always attached to a table it is part of.
        //! Use zero for an object outside a table. There is no default value because
        //! zero is considered as an unusual use case and we want to avoid missing table pointer in
        //! constructors of the various tables.
        //!
        explicit AbstractTableAttachment(const AbstractTable* table) : _table(table) {}

        //!
        //! Get the table id of the parent table.
        //! @return The table id of the parent table or TID_NULL if there is none.
        //!
        TID tableId() const;

        //!
        //! Get the standards of the parent table.
        //! @return The standards of the parent table or NONE if there is none.
        //!
        Standards tableStandards() const;

        //!
        //! Get the parent table.
        //! @return The parent table or zero if there is none.
        //!
        const AbstractTable* table() const { return _table; }

        //!
        //! Check if this instance has a parent table.
        //! @return True if there is one parent table.
        //!
        bool hasTable() const { return _table != nullptr; }

    private:
        const AbstractTable* const _table;
    };
}
