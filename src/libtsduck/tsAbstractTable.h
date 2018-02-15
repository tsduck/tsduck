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
//!
//!  @file
//!  Abstract base class for MPEG PSI/SI tables
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractSignalization.h"
#include "tsTablesPtr.h"
#include "tsMPEG.h"

namespace ts {

    class tsBinaryTable;
    class TablesDisplay;
    class DVBCharset;

    //!
    //! Abstract base class for MPEG PSI/SI tables.
    //!
    class TSDUCKDLL AbstractTable: public AbstractSignalization
    {
    public:
        //!
        //! Get the table_id.
        //! @return The table_id.
        //!
        TID tableId() const {return _table_id;}

        //!
        //! This abstract method serializes a table.
        //! @param [out] bin A binary table object.
        //! Its content is replaced with a binary representation of this object.
        //! @param [in] charset If not zero, default character set to use.
        //!
        virtual void serialize(BinaryTable& bin, const DVBCharset* charset = 0) const = 0;

        //!
        //! This abstract method deserializes a binary table.
        //! In case of success, this object is replaced with the interpreted content of @a bin.
        //! In case of error, this object is invalidated.
        //! @param [in] bin A binary table to interpret according to the table subclass.
        //! @param [in] charset If not zero, character set to use without explicit table code.
        //!
        virtual void deserialize(const BinaryTable& bin, const DVBCharset* charset = 0) = 0;

        //!
        //! Virtual destructor
        //!
        virtual ~AbstractTable() {}



        class AbstractEntry
        {
        public:
            DescriptorList descs;
        protected:
            explicit AbstractEntry(AbstractTable* p) : descs(p) {}
        private:
            AbstractEntry() = delete;
        };

        template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<AbstractEntry, ENTRY>::value>::type* = nullptr>
        class AbstractEntryMap : public std::map<KEY, ENTRY>
        {
        public:
            explicit AbstractEntryMap(AbstractTable* p) : descs(p) {}
            ENTRY& operator[](const KEY& key) { return emplace(key, ENTRY(_p)).first->second; }
        private:
            AbstractTable* const _table;  // Parent table (zero for descriptor list object outside a table).
            AbstractEntryMap() = delete;
        };


    protected:
        //!
        //! The table id can be modified by subclasses only.
        //!
        TID _table_id;

        //!
        //! Protected constructor for subclasses.
        //! @param [in] tid Table id.
        //! @param [in] xml_name Table name, as used in XML structures.
        //!
        AbstractTable(TID tid, const UChar* xml_name);

    private:
        // Unreachable constructors and operators.
        AbstractTable() = delete;
    };
}
