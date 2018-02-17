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
#include "tsDescriptorList.h"
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

        //!
        //! Base inner class for table entries with a descriptor list.
        //!
        //! Some tables, such as PMT, BAT or NIT, contain a list or map of "entries".
        //! Each entry contains a descriptor list. The difficulty here is that the
        //! class DescriptorList needs to be constructed with a reference to a parent
        //! table. The inner class EntryWithDescriptorList can be used as base class
        //! for such entries, combined with the template container classes
        //! EntryWithDescriptorsList and EntryWithDescriptorsMap.
        //!
        class TSDUCKDLL EntryWithDescriptors
        {
        public:
            //!
            //! List of descriptors for this entry, publicly accessible.
            //!
            DescriptorList descs;

            //!
            //! Basic constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //!
            explicit EntryWithDescriptors(const AbstractTable* table);

            //!
            //! Basic copy-like constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //! @param [in] other Another instance to copy.
            //!
            EntryWithDescriptors(const AbstractTable* table, const EntryWithDescriptors& other);

            //!
            //! Assignment operator.
            //! The parent table remains unchanged.
            //! @param [in] other Another instance to copy.
            //! @return A reference to this object.
            //!
            EntryWithDescriptors& operator=(const EntryWithDescriptors& other);

        private:
            // Inaccessible operations.
            EntryWithDescriptors() = delete;
            EntryWithDescriptors(const EntryWithDescriptors&) = delete;
        };

        //!
        //! Template list of subclasses of EntryWithDescriptors.
        //! @tparam ENTRY A subclass of EntryWithDescriptors (enforced at compile-time).
        //!
        template<class ENTRY, typename std::enable_if<std::is_base_of<EntryWithDescriptors, ENTRY>::value>::type* = nullptr>
        class EntryWithDescriptorsList : public std::list<ENTRY>
        {
        public:
            //!
            //! Explicit reference to the super class.
            //!
            typedef std::list<ENTRY> SuperClass;

            //!
            //! Basic constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //!
            explicit EntryWithDescriptorsList(const AbstractTable* table);

            //!
            //! Basic copy-like constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //! @param [in] other Another instance to copy.
            //!
            EntryWithDescriptorsList(const AbstractTable* table, const SuperClass& other);

            //!
            //! Assignment operator.
            //! The parent table remains unchanged.
            //! @param [in] other Another instance to copy.
            //! @return A reference to this object.
            //!
            EntryWithDescriptorsList& operator=(const EntryWithDescriptorsList& other);

            //!
            //! Swap two instances (override of std::list).
            //! @param [in,out] other Another instance to swap with the current object.
            //!
            void swap(EntryWithDescriptorsList& other);

            //!
            //! Create a new entry at front of the list and return a reference to it.
            //! @return A reference to the newly created element.
            //!
            ENTRY& newFront();

            //!
            //! Create a new entry at the back of the list and return a reference to it.
            //! @return A reference to the newly created element.
            //!
            ENTRY& newBack();

        private:
            // Parent table (zero for descriptor list object outside a table).
            const AbstractTable* const _table;

            // Inaccessible operations.
            EntryWithDescriptorsList() = delete;
            EntryWithDescriptorsList(const EntryWithDescriptorsList&) = delete;
        };

        //!
        //! Template map of subclasses of EntryWithDescriptors.
        //! @tparam KEY A type which is used as key of the map.
        //! @tparam ENTRY A subclass of EntryWithDescriptors (enforced at compile-time).
        //!
        template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<EntryWithDescriptors, ENTRY>::value>::type* = nullptr>
        class EntryWithDescriptorsMap : public std::map<KEY, ENTRY>
        {
        public:
            //!
            //! Explicit reference to the super class.
            //!
            typedef std::map<KEY, ENTRY> SuperClass;

            //!
            //! Basic constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //!
            explicit EntryWithDescriptorsMap(const AbstractTable* table);

            //!
            //! Basic copy-like constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //! @param [in] other Another instance to copy.
            //!
            EntryWithDescriptorsMap(const AbstractTable* table, const SuperClass& other);

            //!
            //! Assignment operator.
            //! The parent table remains unchanged.
            //! @param [in] other Another instance to copy.
            //! @return A reference to this object.
            //!
            EntryWithDescriptorsMap& operator=(const EntryWithDescriptorsMap& other);

            //!
            //! Swap two instances (override of std::list).
            //! @param [in,out] other Another instance to swap with the current object.
            //!
            void swap(EntryWithDescriptorsMap& other);

            //!
            //! Access or create an entry.
            //! @param [in] key The key of the entry to access.
            //! @return A reference to the retrieved or created entry.
            //!
            ENTRY& operator[](const KEY& key);

            //!
            //! Access an existing entry in a read-only map.
            //! @param [in] key The key of the entry to access.
            //! @return A constant reference to the retrieved entry.
            //! @throw std::out_of_range When the entry does not exist.
            //!
            const ENTRY& operator[](const KEY& key) const;

        private:
            // Parent table (zero for descriptor list object outside a table).
            const AbstractTable* const _table;

            // Inaccessible operations.
            EntryWithDescriptorsMap() = delete;
            EntryWithDescriptorsMap(const EntryWithDescriptorsMap&) = delete;
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

#include "tsAbstractTableTemplate.h"
