//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
    //!
    //! Abstract base class for MPEG PSI/SI tables.
    //! @ingroup table
    //!
    //! Some methods are declared as "virtual final". Since these methods are not
    //! inherited, this seems useless. This is in fact a compilation check. These
    //! methods were formerly designed to be overridden by subclasses but the
    //! implementation has changed. They are now defined in this class only and
    //! call a new pure virtual method. The "final" attribute is here to detect
    //! old subclasses which do not yet use the new scheme.
    //!
    //! A table subclass shall override the following methods:
    //! - clearContent()
    //! - tableIdExtension() : for long tables only, see AbstractLongTable
    //! - serializePayload()
    //! - deserializePayload()
    //! - buildXML()
    //! - analyzeXML()
    //!
    //! A table subclass may also override the following methods when necessary:
    //! - isPrivate() : for non-private table, ie. MPEG-defined or SCTE-defined.
    //! - isValidTableId() : for table types accepting several table id values.
    //!
    class TSDUCKDLL AbstractTable: public AbstractSignalization
    {
    public:
        //!
        //! Get the table_id.
        //! @return The table_id.
        //!
        TID tableId() const { return _table_id; }

        //!
        //! Check if the table is a private one (ie. not MPEG-defined).
        //! The default implementation returns true. MPEG-defined tables should override this method to return false.
        //! @return True if the table is a private one (ie. not MPEG-defined).
        //!
        virtual bool isPrivate() const;

        //!
        //! This method serializes a table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [out] bin A binary table object.
        //! Its content is replaced with a binary representation of this object.
        //!
        virtual void serialize(DuckContext& duck, BinaryTable& bin) const final;

        //!
        //! This method deserializes a binary table.
        //! In case of success, this object is replaced with the interpreted content of @a bin.
        //! In case of error, this object is invalidated.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary table to interpret according to the table subclass.
        //!
        virtual void deserialize(DuckContext& duck, const BinaryTable& bin) final;

        //!
        //! Virtual destructor
        //!
        virtual ~AbstractTable();

        //!
        //! Base inner class for table entries with one or more descriptor lists.
        //!
        class TSDUCKDLL EntryBase
        {
        };

        //!
        //! Base inner class for table entries with one descriptor list.
        //!
        //! Some tables, such as PMT, BAT or NIT, contain a list or map of "entries".
        //! Each entry contains a descriptor list. The difficulty here is that the
        //! class DescriptorList needs to be constructed with a reference to a parent
        //! table. The inner class EntryWithDescriptorList can be used as base class
        //! for such entries, combined with the template container classes
        //! EntryWithDescriptorsList and EntryWithDescriptorsMap.
        //!
        class TSDUCKDLL EntryWithDescriptors : public EntryBase
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

            //!
            //! Move assignment operator.
            //! The parent table remains unchanged.
            //! @param [in,out] other Another instance to copy.
            //! @return A reference to this object.
            //!
            EntryWithDescriptors& operator=(EntryWithDescriptors&& other) noexcept;

        private:
            // Inaccessible operations.
            EntryWithDescriptors() = delete;
            EntryWithDescriptors(EntryWithDescriptors&&) = delete;
            EntryWithDescriptors(const EntryWithDescriptors&) = delete;
        };

        //!
        //! Template map of subclasses of EntryBase.
        //! @tparam KEY A type which is used as key of the map.
        //! @tparam ENTRY A subclass of EntryBase (enforced at compile-time).
        //!
        template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<EntryBase, ENTRY>::value>::type* = nullptr>
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
            //! Move assignment operator.
            //! The parent table remains unchanged.
            //! @param [in,out] other Another instance to copy.
            //! @return A reference to this object.
            //!
            EntryWithDescriptorsMap& operator=(EntryWithDescriptorsMap&& other);

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
            EntryWithDescriptorsMap(EntryWithDescriptorsMap&&) = delete;
            EntryWithDescriptorsMap(const EntryWithDescriptorsMap&) = delete;
        };

        //!
        //! Template map of subclasses of EntryBase, indexed by size_t.
        //! This is replacement for vectors and lists, which cannot be used by entries
        //! containing a descriptor list since it is not CopyAssignable or CopyConstructible.
        //! @tparam ENTRY A subclass of EntryBase (enforced at compile-time).
        //!
        template<class ENTRY, typename std::enable_if<std::is_base_of<EntryBase, ENTRY>::value>::type* = nullptr>
        class EntryWithDescriptorsList : public EntryWithDescriptorsMap<size_t, ENTRY>
        {
        public:
            //!
            //! Explicit reference to the super class.
            //!
            typedef EntryWithDescriptorsMap<size_t, ENTRY> SuperClass;

            //!
            //! Basic constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //!
            explicit EntryWithDescriptorsList(const AbstractTable* table) : SuperClass(table) {}

            //!
            //! Basic copy-like constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //! @param [in] other Another instance to copy.
            //!
            EntryWithDescriptorsList(const AbstractTable* table, const SuperClass& other) : SuperClass(table, other) {}

            //!
            //! Get a new unused index, greater than the greatest entry.
            //! @return A new unused index.
            //!
            size_t nextIndex() const { return this->empty() ? 0 : this->rbegin()->first + 1; }

            //!
            //! Create a new entry in the map.
            //! @return A constant reference to the new entry.
            //!
            ENTRY& newEntry() { return (*this)[this->nextIndex()]; }
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
        //! @param [in] standards A bit mask of standards which define this structure.
        //! @param [in] xml_legacy_name Table or descriptor legacy XML name. Ignored if null pointer.
        //!
        AbstractTable(TID tid, const UChar* xml_name, Standards standards, const UChar* xml_legacy_name = nullptr);

        //!
        //! This method checks if a table id is valid for this object.
        //! @param [in] tid A table id to check.
        //! @return True if @a tid is a valid table id for this object, false otherwise.
        //! The default implementation checks that @a tid is identical to the table id
        //! of this object.
        //!
        virtual bool isValidTableId(TID tid) const;

        //!
        //! This abstract method serializes the content of a table.
        //! This method is invoked by serialize() when the table is valid.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [out] bin A binary table object.
        //! Its content is replaced with a binary representation of this object.
        //!
        virtual void serializeContent(DuckContext& duck, BinaryTable& bin) const;

        //!
        //! This method deserializes the content of a binary table.
        //!
        //! In case of success, this object is replaced with the interpreted content of @a bin.
        //! In case of error, this object is invalidated.
        //!
        //! The subclass shall preferably override deserializePayload(). As legacy, the subclass may directly override
        //! deserializeContent() but this is not recommended for new tables. At some point, if we can refactor all
        //! tables to the new scheme using deserializePayload(), deserializeContent() will disappear or
        //! become "final" and will no longer allow override.
        //!
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary table to interpret according to the table subclass.
        //!
        virtual void deserializeContent(DuckContext& duck, const BinaryTable& bin);

        //!
        //! This abstract method serializes the payload of all sections in the table.
        //!
        //! When serialize() is called, the output binary table is cleared and serializePayload()
        //! is called. A subclass shall implement serializePayload() which adds all required sections
        //! in the binary table.
        //!
        //! Note that it is not necessary to explicitly add the last (or only) section. Upon return from
        //! serializePayload(), serialize() checks the state of the @a payload buffer. If the output
        //! binary table is still empty or if the @a payload buffer is not empty (or not empty after
        //! the last saved write position), then addOneSection() is automatically called.
        //!
        //! This is now the preferred method for table serialization: use the default implementation
        //! of serializeContent() and let it call the overridden serializePayload().
        //!
        //! The default implementation generates an error. So, if a subclass overrides neither serializeContent()
        //! nor serializePayload(), all serialization will fail.
        //!
        //! @param [in,out] table The binary table into which this object shall be serialized. The @a table is
        //! initially empty when serialize() calls serializePayload().
        //! @param [in,out] payload A PSIBuffer with the appropriate size for the section payload. The @a payload
        //! buffer is initially empty when serialize() calls serializePayload().
        //!
        virtual void serializePayload(BinaryTable& table, PSIBuffer& payload) const;

        //!
        //! This abstract method deserializes the payload of a section.
        //!
        //! When deserialize() is called, this object is cleared and validated. Then, deserializePayload()
        //! is invoked for each section in the binary table. A subclass shall implement deserializePayload()
        //! which adds the content of the binary section to the C++ object. Do not reset the object in
        //! deserializePayload() since it is repeatedly called for each section of a single binary table.
        //!
        //! This is now the preferred method for table deserialization: use the default implementation
        //! of deserializeContent() and let it call the overridden deserializePayload().
        //!
        //! The default implementation generates an error. So, if a subclass overrides neither deserializeContent()
        //! nor deserializePayload(), all deserialization will fail.
        //!
        //! @param [in,out] buf Deserialization buffer. The subclass shall read the descriptor payload from
        //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
        //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
        //! @param [in] section A reference to the section. Can be used to access values in the section header
        //! (typically for long sections).
        //!
        virtual void deserializePayload(PSIBuffer& buf, const Section& section);

        //!
        //! Helper method for serializePayload(): add a section in a binary table.
        //!
        //! For long tables, the section number is always one more than the current last section in the table.
        //!
        //! It the @a payload buffer has a pushed read/write state, this state is restored and immediately pushed again.
        //! The typical use case is the following:
        //! - A table may create more than one section.
        //! - The payload of all sections starts with the same fixed data.
        //! - In the subclass, the method serializePayload() builds the initial fixed data once.
        //! - The method serializePayload() immediately pushes the read/write state of the buffer.
        //! - The method serializePayload() builds payloads and call addOneSection().
        //! - Upon return from addOneSection(), the buffer is back right after the initial fixed data.
        //!
        //! @param [in,out] table The binary table into which the new section shall be added.
        //! @param [in,out] payload A PSIBuffer containing the section payload between the read and the write pointer.
        //!
        void addOneSection(BinaryTable& table, PSIBuffer& payload) const;

        //!
        //! Actual implementation of adding one section in a binary table.
        //! Do not call directly, it is only called by addOneSection() and is overridden in AbstractLongTable.
        //! @param [in,out] table The binary table into which the new section shall be added.
        //! @param [in,out] payload A PSIBuffer containing the section payload between the read and the write pointer.
        //!
        virtual void addOneSectionImpl(BinaryTable& table, PSIBuffer& payload) const;

        //!
        //! Wrapper for deserializePayload().
        //! This is a method to overload in intermediate classes to avoid using "call superclass" to all tables.
        //! @param [in,out] buf Deserialization buffer.
        //! @param [in] section A reference to the section.
        //!
        virtual void deserializePayloadWrapper(PSIBuffer& buf, const Section& section);

        //!
        //! Get the maximum size in bytes of the payload of sections of this table.
        //! @return The maximum size in bytes of the payload of sections of this table.
        //!
        virtual size_t maxPayloadSize() const;

    private:
        // Unreachable constructors and operators.
        AbstractTable() = delete;
    };
}

#include "tsAbstractTableTemplate.h"
