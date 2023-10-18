//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#include "tsSection.h"
#include "tsDescriptorList.h"
#include "tsAlgorithm.h"

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
        TS_RULE_OF_FIVE(AbstractTable, override);
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
        //! @return True in case of success, false if the table is invalid.
        //!
        bool serialize(DuckContext& duck, BinaryTable& bin) const;

        //!
        //! This method deserializes a binary table.
        //! In case of success, this object is replaced with the interpreted content of @a bin.
        //! In case of error, this object is invalidated.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary table to interpret according to the table subclass.
        //! @return True in case of success, false if the table is invalid.
        //!
        bool deserialize(DuckContext& duck, const BinaryTable& bin);

        //!
        //! Base inner class for table entries with one or more descriptor lists.
        //!
        class TSDUCKDLL EntryBase
        {
        public:
            //!
            //! Preferred insertion index when serializing the table or NPOS if unspecified.
            //! This is an informational hint which can be used or ignored.
            //!
            size_t order_hint;

            //!
            //! Default constructor.
            //! @param [in] order Ordering hint, unspecified by default.
            //!
            explicit EntryBase(size_t order = NPOS) : order_hint(order) {}
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
            //! Basic move-like constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //! @param [in,out] other Another instance to move.
            //!
            EntryWithDescriptors(const AbstractTable* table, EntryWithDescriptors&& other);

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
            //! @param [in,out] other Another instance to move.
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
            //! @param [in] auto_ordering If true, each time an entry is added, its @a order_hint, if previously unset
            //! (meaning set to NPOS) is set to one higher than the highest @a order_hint in all entries. This ensures
            //! that the order of insertion is preserved, at the expense of a small performance penalty
            //! each time an entry is added.
            //!
            explicit EntryWithDescriptorsMap(const AbstractTable* table, bool auto_ordering = false);

            //!
            //! Basic copy-like constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //! @param [in] other Another instance to copy.
            //!
            EntryWithDescriptorsMap(const AbstractTable* table, const EntryWithDescriptorsMap& other);

            //!
            //! Basic move-like constructor.
            //! @param [in] table Parent table. A descriptor list is always attached to a table.
            //! @param [in,out] other Another instance to move.
            //!
            EntryWithDescriptorsMap(const AbstractTable* table, EntryWithDescriptorsMap&& other);

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
            //! @param [in,out] other Another instance to move.
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

            //!
            //! Get the insertion order of entries in the table.
            //! The result is based on the @a order_hint fields in the EntryBase structures.
            //! @param [out] order Order of entries by key in the table.
            //!
            void getOrder(std::vector<KEY>& order) const;

            //!
            //! Define the insertion order of entries in the table.
            //! This can be precisely set using the @a order_hint fields in the EntryBase structures.
            //! This method is a helper which sets these fields.
            //! @param [in] order Order of entries by key in the table.
            //!
            void setOrder(const std::vector<KEY>& order);

            //!
            //! Get the next ordering hint to be used in an entry to make sure it is considered the last one.
            //! @return The next ordering hint to be used.
            //!
            size_t nextOrder() const;

        private:
            // Parent table (zero for descriptor list object outside a table).
            const AbstractTable* const _table;
            bool _auto_ordering = false;

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
        TID _table_id = TID_NULL;

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
        //! Get the maximum size in bytes of the payload of sections of this table.
        //! @return The maximum size in bytes of the payload of sections of this table.
        //!
        virtual size_t maxPayloadSize() const;

        //!
        //! Check if the sections of this table have a trailing CRC32.
        //! This is usually false for short sections but some short sections such as DVB-TOT use a CRC32.
        //! @return True if the sections of this table have a trailing CRC32.
        //!
        virtual bool useTrailingCRC32() const;

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
        //! @param [in,out] table The binary table into which this object shall be serialized. The @a table is
        //! initially empty when serialize() calls serializePayload().
        //! @param [in,out] buf A PSIBuffer with the appropriate size for the section payload. The @a payload
        //! buffer is initially empty when serialize() calls serializePayload().
        //!
        virtual void serializePayload(BinaryTable& table, PSIBuffer& buf) const = 0;

        //!
        //! This abstract method deserializes the payload of one section.
        //!
        //! When deserialize() is called, this object is cleared and validated. Then, deserializePayload()
        //! is invoked for each section in the binary table. A subclass shall implement deserializePayload()
        //! which adds the content of the binary section to the C++ object. Do not reset the object in
        //! deserializePayload() since it is repeatedly called for each section of a single binary table.
        //!
        //! @param [in,out] buf Deserialization buffer. The subclass shall read the descriptor payload from
        //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
        //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
        //! @param [in] section A reference to the section. Can be used to access values in the section header
        //! (typically for long sections).
        //!
        virtual void deserializePayload(PSIBuffer& buf, const Section& section) = 0;

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

    private:
        // Unreachable constructors and operators.
        AbstractTable() = delete;
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>::EntryWithDescriptorsMap(const AbstractTable* table, bool auto_ordering) :
    SuperClass(),
    _table(table),
    _auto_ordering(auto_ordering)
{
}

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>::EntryWithDescriptorsMap(const AbstractTable* table, const EntryWithDescriptorsMap& other) :
    SuperClass(),
    _table(table),
    _auto_ordering(other._auto_ordering)
{
    // Copy each entry one by one to ensure that the copied entries actually point to the constructed table.
    for (auto& it : other) {
        (*this)[it.first] = it.second;
    }
}

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
ts::AbstractTable::EntryWithDescriptorsMap<KEY, ENTRY, N>::EntryWithDescriptorsMap(const AbstractTable* table, EntryWithDescriptorsMap&& other) :
    SuperClass(),
    _table(table),
    _auto_ordering(other._auto_ordering)
{
    // Clear and move each entry one by one to ensure that the copied entries actually point to the target table.
    for (auto& it : other) {
        (*this)[it.first] = std::move(it.second);
    }
    // The other instance is still a valid map with valid entries.
    // But all entries have unspecified values.
    // Just clear the other it to get a deterministic state.
    other.clear();
}

//----------------------------------------------------------------------------
// Template list of subclasses of EntryWithDescriptors - Assignment.
//----------------------------------------------------------------------------

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>& ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>::operator=(const EntryWithDescriptorsMap& other)
{
    if (&other != this) {
        // Use same auto ordering as copied map.
        _auto_ordering = other._auto_ordering;
        // Clear and copy each entry one by one to ensure that the copied entries actually point to the target table.
        this->clear();
        for (auto& it : other) {
            (*this)[it.first] = it.second;
        }
    }
    return *this;
}

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
ts::AbstractTable::EntryWithDescriptorsMap<KEY, ENTRY, N>& ts::AbstractTable::EntryWithDescriptorsMap<KEY, ENTRY, N>::operator=(EntryWithDescriptorsMap&& other)
{
    if (&other != this) {
        // Use same auto ordering as copied map.
        _auto_ordering = other._auto_ordering;
        // Clear and move each entry one by one to ensure that the copied entries actually point to the target table.
        this->clear();
        for (auto& it : other) {
            (*this)[it.first] = std::move(it.second);
        }
        // The other instance is still a valid map with valid entries.
        // But all entries have unspecified values.
        // Just clear the other it to get a deterministic state.
        other.clear();
    }
    return *this;
}

//----------------------------------------------------------------------------
// Template map of subclasses of EntryWithDescriptors - Swap.
//----------------------------------------------------------------------------

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
void ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>::swap(EntryWithDescriptorsMap& other)
{
    if (&other != this) {
        // Unefficient but functionally correct.
        const EntryWithDescriptorsMap tmp(nullptr, other);
        other = *this;
        *this = tmp;
    }
}

//----------------------------------------------------------------------------
// Template map of subclasses of EntryWithDescriptors - Subscripts.
//----------------------------------------------------------------------------

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
ENTRY& ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>::operator[](const KEY& key)
{
    // The emplace operation ensures that the object is constructed with the supplied arguments (and not copied).
    // This form is more complex but the simplest form of emplace (without the piecewise_construct and tuples)
    // does not compile with LLVM. Not sure if this is a problem with LLVM or if the other compilers are too
    // permissive.
    ENTRY& entry(this->emplace(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(_table)).first->second);

    // When not already specified otherwise, keep the order of entry creation.
    if (_auto_ordering && entry.order_hint == NPOS) {
        entry.order_hint = this->nextOrder();
    }
    return entry;
}

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
const ENTRY& ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>::operator[](const KEY& key) const
{
    // Here, we must not create any element (the instance is read-only).
    const auto it = this->find(key);
    if (it == this->end()) {
        // Same exception as std::map::at().
        throw std::out_of_range("unknown key in ts::AbstractTable::EntryWithDescriptorsMap");
    }
    return it->second;
}


//----------------------------------------------------------------------------
// Template map of subclasses of EntryWithDescriptors.
// Get the insertion order of entrie in the table.
//----------------------------------------------------------------------------

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
void ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>::getOrder(std::vector<KEY>& order) const
{
    // Build a multimap of keys, indexed by order_hint.
    std::multimap<size_t, KEY> kmap;
    for (const auto& entry : *this) {
        kmap.insert(std::make_pair(entry.second.order_hint, entry.first));
    }

    // Build a vector of keys from this order.
    order.clear();
    order.reserve(kmap.size());
    for (const auto& it : kmap) {
        order.push_back(it.second);
    }
}


//----------------------------------------------------------------------------
// Template map of subclasses of EntryWithDescriptors.
// Set the insertion order of entrie in the table.
//----------------------------------------------------------------------------

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
void ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>::setOrder(const std::vector<KEY>& order)
{
    // First pass: get initial ordering.
    std::vector<KEY> input;
    this->getOrder(input);

    // Second pass: Assign ordering hints to explicitly sorted keys.
    size_t count = 0;
    for (const KEY& key : order) {
        const auto it = this->find(key);
        if (it != this->end()) {
            it->second.order_hint = count++;
        }
    }

    // Third pass: reassign increasing ordering numbers for unspecified keys, same order as previously.
    for (const KEY& key : input) {
        if (!Contains(order, key)) {
            const auto it = this->find(key);
            if (it != this->end()) {
                it->second.order_hint = count++;
            }
        }
    }
}

//----------------------------------------------------------------------------
// Template map of subclasses of EntryWithDescriptors.
// Get the next ordering hint to be used in an entry.
//----------------------------------------------------------------------------

template<typename KEY, class ENTRY, typename std::enable_if<std::is_base_of<ts::AbstractTable::EntryBase, ENTRY>::value>::type* N>
size_t ts::AbstractTable::EntryWithDescriptorsMap<KEY,ENTRY,N>::nextOrder() const
{
    size_t next = 0;
    for (const auto& entry : *this) {
        if (entry.second.order_hint != NPOS) {
            next = std::max(next, entry.second.order_hint + 1);
        }
    }
    return next;
}
