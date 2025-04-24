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
#include "tsAbstractTableAttachment.h"
#include "tsDescriptor.h"
#include "tsEDID.h"
#include "tsDescriptorContext.h"

namespace ts {

    class AbstractTable;
    class AbstractDescriptor;
    class DuckContext;

    //!
    //! List of MPEG PSI/SI descriptors.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL DescriptorList : public AbstractTableAttachment
    {
        TS_NO_DEFAULT_CONSTRUCTORS(DescriptorList);
    public:
        //!
        //! Basic constructor.
        //! @param [in] table Parent table. A descriptor list is always attached to a table it is part of.
        //! Use zero for a descriptor list object outside a table. There is no default value because
        //! zero is considered as an unusual use case and we want to avoid missing table pointer in
        //! constructors of the various tables.
        //!
        explicit DescriptorList(const AbstractTable* table);

        //!
        //! Basic copy-like constructor.
        //! We forbid a real copy constructor because we want to copy the descriptors only,
        //! while the parent table is usually different.
        //! The descriptors objects are shared between the two lists.
        //! @param [in] table Parent table. A descriptor list is always attached to a table it is part of.
        //! Use zero for a descriptor list object outside a table.
        //! @param [in] dl Another instance to copy.
        //!
        DescriptorList(const AbstractTable* table, const DescriptorList& dl);

        //!
        //! Basic move-like constructor.
        //! We forbid a real move constructor because we want to copy the descriptors only,
        //! while the parent table is usually different.
        //! @param [in] table Parent table. A descriptor list is always attached to a table it is part of.
        //! Use zero for a descriptor list object outside a table.
        //! @param [in,out] dl Another instance to move.
        //!
        DescriptorList(const AbstractTable* table, DescriptorList&& dl) noexcept;

        //!
        //! Assignment operator.
        //! The descriptors objects are shared between the two lists.
        //! The parent table remains unchanged.
        //! @param [in] dl Another instance to copy.
        //! @return A reference to this object.
        //!
        DescriptorList& operator=(const DescriptorList& dl);

        //!
        //! Move assignment operator.
        //! The descriptors objects are moved.
        //! The parent table remains unchanged.
        //! @param [in,out] dl Another instance to move.
        //! @return A reference to this object.
        //!
        DescriptorList& operator=(DescriptorList&& dl) noexcept;

        //!
        //! Check if the descriptor list is empty.
        //! @return True if the descriptor list is empty.
        //!
        bool empty() const { return _list.empty(); }

        //!
        //! Get the number of descriptors in the list (same as count()).
        //! @return The number of descriptors in the list.
        //!
        size_t size() const { return _list.size(); }

        //!
        //! Get the number of descriptors in the list (same as size()).
        //! @return The number of descriptors in the list.
        //!
        size_t count() const { return _list.size(); }

        //!
        //! Comparison operator.
        //! @param [in] other Another instance to compare.
        //! @return True if the two descriptor lists are identical.
        //!
        bool operator==(const DescriptorList& other) const;

        //!
        //! Get a reference to the descriptor at a specified index.
        //! @param [in] index Index in the list. Valid index are 0 to count()-1.
        //! @return A reference to the descriptor at @a index.
        //!
        Descriptor& operator[](size_t index);

        //!
        //! Get a const reference to the descriptor at a specified index.
        //! @param [in] index Index in the list. Valid index are 0 to count()-1.
        //! @return A const reference to the descriptor at @a index.
        //!
        const Descriptor& operator[](size_t index) const;

        //!
        //! An iterator over binary descriptors in the list.
        //! Dereferencing an iterator accesses a Descriptor instance.
        //!
        class TSDUCKDLL iterator : private std::vector<DescriptorPtr>::iterator
        {
        private:
            using SuperClass = std::vector<DescriptorPtr>::iterator;
            friend class DescriptorList;
            iterator(const SuperClass& up) : SuperClass(up) {}
        public:
            //! @cond nodoxygen
            iterator& operator--() { --*static_cast<SuperClass*>(this); return *this; }
            iterator& operator++() { ++*static_cast<SuperClass*>(this); return *this; }
            Descriptor& operator*() { return **static_cast<SuperClass>(*this); }
            Descriptor* operator->() { return &**static_cast<SuperClass>(*this); }
            bool operator==(const iterator& other) const { return static_cast<SuperClass>(*this) == static_cast<SuperClass>(other); }
            //! @endcond
        };

        //!
        //! A constant iterator over binary descriptors in the list.
        //! Dereferencing an iterator accesses a constant Descriptor instance.
        //!
        class TSDUCKDLL const_iterator : private std::vector<DescriptorPtr>::const_iterator
        {
        private:
            using SuperClass = std::vector<DescriptorPtr>::const_iterator;
            friend class DescriptorList;
            const_iterator(const SuperClass& up) : SuperClass(up) {}
        public:
            //! @cond nodoxygen
            const_iterator& operator--() { --*static_cast<SuperClass*>(this); return *this; }
            const_iterator& operator++() { ++*static_cast<SuperClass*>(this); return *this; }
            const Descriptor& operator*() { return **static_cast<SuperClass>(*this); }
            const Descriptor* operator->() { return &**static_cast<SuperClass>(*this); }
            bool operator==(const const_iterator& other) const { return static_cast<SuperClass>(*this) == static_cast<SuperClass>(other); }
            //! @endcond
        };

        //!
        //! Get an iterator to the first descriptor in the list.
        //! @return An iterator to the first descriptor in the list.
        //!
        iterator begin() { return static_cast<iterator>(_list.begin()); }

        //!
        //! Get an iterator after the last descriptor in the list.
        //! @return An iterator after the last descriptor in the list.
        //!
        iterator end() { return static_cast<iterator>(_list.end()); }

        //!
        //! Get a constant iterator to the first descriptor in the list.
        //! @return A constant iterator to the first descriptor in the list.
        //!
        const_iterator begin() const { return static_cast<const_iterator>(_list.begin()); }

        //!
        //! Get a constant iterator after the last descriptor in the list.
        //! @return A constant iterator after the last descriptor in the list.
        //!
        const_iterator end() const { return static_cast<const_iterator>(_list.end()); }

        //!
        //! Get the extended descriptor id of a descriptor in the list.
        //! @param [in] duck TSDuck execution context.
        //! @param [in] index Index of a descriptor in the list. Valid index are 0 to count()-1.
        //! @return The extended descriptor id at @a index.
        //!
        EDID edid(const DuckContext& duck, size_t index) const;

        //!
        //! Return the MPEG "registration id" associated to a descriptor in the list.
        //! @param [in] index Index of a descriptor in the list. Valid index are 0 to count()-1.
        //! @return The "registration id" associated to the descriptor at @a index or REGID_NULL.
        //!
        REGID registrationId(size_t index) const;

        //!
        //! Return the "private data specifier" associated to a descriptor in the list.
        //! @param [in] index Index of a descriptor in the list. Valid index are 0 to count()-1.
        //! @return The "private data specifier" associated to a descriptor at @a index or PDS_NULL.
        //!
        PDS privateDataSpecifier(size_t index) const;

        //!
        //! Add one descriptor at end of list.
        //! The descriptor content is shared.
        //! @param [in] desc The binary descriptor to add.
        //! @return True in case of success, false if the descriptor is invalid.
        //!
        bool add(const DescriptorPtr& desc);

        //!
        //! Add one descriptor at end of list.
        //! The descriptor content is copied.
        //! @param [in] desc The binary descriptor to add.
        //! @return True in case of success, false if the descriptor is invalid.
        //!
        bool add(const Descriptor& desc);

        //!
        //! Add one descriptor at end of list
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] desc The descriptor to add.
        //! @return True in case of success, false if the descriptor is invalid.
        //!
        bool add(DuckContext& duck, const AbstractDescriptor& desc);

        //!
        //! Add another list of descriptors at end of list.
        //! The descriptors objects are shared between the two lists.
        //! @param [in] dl The descriptor list to add.
        //!
        void add(const DescriptorList& dl)
        {
            _list.insert(_list.end(), dl._list.begin(), dl._list.end());
        }

        //!
        //! Add descriptors from a memory area at end of list
        //! @param [in] addr Address of descriptors in memory.
        //! @param [in] size Size in bytes of descriptors in memory.
        //! @return True in case of success, false in case of invalid or truncated descriptor.
        //!
        bool add(const void* addr, size_t size);

        //!
        //! Add one descriptor from a memory area at end of list.
        //! The size is extracted from the descriptor header.
        //! @param [in] addr Address of the descriptor in memory.
        //! @return True in case of success, false if the descriptor is invalid.
        //!
        bool add(const void* addr);

        //!
        //! Add a MPEG registration_descriptor if necessary at end of list.
        //! If the current registration at end of list is not @a regid,
        //! a registration_descriptor is added. If @a regid is already
        //! the current registration id, the list is unchanged.
        //! @param [in] regid A registration id.
        //!
        void addRegistration(REGID regid);

        //!
        //! Add a DVB private_data_specifier_descriptor if necessary at end of list.
        //! If the current private data specifier at end of list is not @a pds,
        //! a private_data_specifier descriptor is added. If @a pds is already
        //! the current private data specifier, the list is unchanged.
        //! @param [in] pds A private data specifier.
        //!
        void addPrivateDataSpecifier(PDS pds);

        //!
        //! Add a MPEG registration_descriptor or a DVB private_data_specifier_descriptor if necessary at end of list.
        //! @param [in] edid Extended descriptor id of the descriptor to add and may need an preceding
        //! MPEG registration_descriptor or a DVB private_data_specifier_descriptor.
        //!
        void addPrivateIdentifier(EDID edid);

        //!
        //! Check if the descriptor list contains a MPEG registration descriptor with the specified id.
        //! @param [in] regid The registration id to check.
        //! @return True if @a regid is present in a registration descriptor.
        //!
        bool containsRegistration(REGID regid) const;

        //!
        //! Get a list of all registration ids, in all MPEG registration descriptors.
        //! @param [in] duck TSDuck execution context.
        //! @param [out] regids A list of all registration ids, in their order of appearance.
        //! The returned list can contain duplicates if the duplicates are present in the descriptor list.
        //!
        void getAllRegistrations(const DuckContext& duck, REGIDVector& regids) const;

        //!
        //! Get a list of all language codes from all descriptors.
        //! @param [in] duck TSDuck execution context.
        //! @param [out] languages A list of all language codes, in their order of appearance.
        //! The returned list can contain duplicates if the duplicates are present in the descriptor list.
        //! @param [in] max_count The maximum number of languages to return. By default, return them all.
        //!
        void getAllLanguages(const DuckContext& duck, UStringVector& languages, size_t max_count = NPOS) const;

        //!
        //! Merge one descriptor in the list.
        //! If a descriptor of the same type is already present in the list,
        //! the DescriptorDuplication mode of the descriptor class is applied.
        //! If there is no descriptor of the same type, the descriptor is added
        //! at the end of the list.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] desc The descriptor to merge.
        //! @return True in case of success, false if the descriptor is invalid.
        //!
        bool merge(DuckContext& duck, const AbstractDescriptor& desc);

        //!
        //! Merge another descriptor list in this list.
        //! All descriptors are merged one by one.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] other The other descriptor list to merge.
        //!
        void merge(DuckContext& duck, const DescriptorList& other);

        //!
        //! Remove the descriptor at the specified index in the list.
        //! A private_data_specifier descriptor can be removed only if
        //! it is not necessary (no private descriptor ahead).
        //! @param [in] index Index of the descriptor to remove.
        //! @return True on success, false on error (index out of range
        //! or required private_data_specifier descriptor).
        //!
        bool removeByIndex(size_t index);

        //!
        //! Remove all descriptors with the specified tag.
        //! A private_data_specifier descriptor can be removed only if
        //! it is not necessary (no private descriptor ahead).
        //! @param [in] tag Tag of descriptors to remove.
        //! @param [in] pds Private data specifier.
        //! If @a pds is non-zero and @a tag is >= 0x80, remove only
        //! descriptors with the corresponding private data specifier.
        //! @return The number of removed descriptors.
        //!
        size_t removeByTag(DID tag, PDS pds = 0);

        //!
        //! Remove all DVB private descriptors without preceding private_data_specifier_descriptor.
        //! @return The number of removed descriptors.
        //!
        size_t removeInvalidPrivateDescriptors();

        //!
        //! Clear the content of the descriptor list.
        //!
        void clear() { _list.clear(); }

        //!
        //! Search a descriptor with the specified tag.
        //! @param [in] tag Tag of descriptor to search.
        //! @param [in] start_index Start searching at this index.
        //! @param [in] pds Private data specifier.
        //! If @a pds is non-zero and @a tag is >= 0x80, return only
        //! a descriptor with the corresponding private data specifier.
        //! @return The index of the descriptor in the list or count() if no such descriptor is found.
        //!
        size_t search(DID tag, size_t start_index = 0, PDS pds = 0) const;

        //!
        //! Search a descriptor with the specified extended tag.
        //! @param [in] edid Extended tag of descriptor to search.
        //! @param [in] start_index Start searching at this index.
        //! @return The index of the descriptor in the list or count() if no such descriptor is found.
        //!
        size_t search(const EDID& edid, size_t start_index = 0) const;

        //!
        //! Search a descriptor for the specified language.
        //! This can be an audio, subtitles or other component descriptor.
        //! @param [in] duck TSDuck execution context.
        //! @param [in] language The 3-character language name to search.
        //! @param [in] start_index Start searching at this index.
        //! @return The index of the descriptor in the list or count() if no such descriptor is found.
        //!
        size_t searchLanguage(const DuckContext& duck, const UString& language, size_t start_index = 0) const;

        //!
        //! Search any kind of subtitle descriptor.
        //! @param [in] duck TSDuck execution context.
        //! @param [in] language The language name to search.
        //! If @a language is non-empty, look only for a subtitle
        //! descriptor matching the specified language. In this case, if some
        //! kind of subtitle descriptor exists in the list but none matches the
        //! language, return count()+1.
        //! @param [in] start_index Start searching at this index.
        //! @return The index of the descriptor in the list or count() if no such descriptor is found.
        //!
        size_t searchSubtitle(const DuckContext& duck, const UString& language = UString(), size_t start_index = 0) const;

        //!
        //! Search a descriptor with the specified tag.
        //! @tparam DESC A subclass of AbstractDescriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] tag Tag of descriptor to search.
        //! @param [out] desc When a descriptor with the specified tag is found,
        //! it is deserialized into @a desc. Always check desc.isValid() on return
        //! to check if the deserialization was successful.
        //! @param [in] start_index Start searching at this index.
        //! @param [in] pds Private data specifier.
        //! If @a pds is non-zero and @a tag is >= 0x80, return only
        //! a descriptor with the corresponding private data specifier.
        //! @return The index of the descriptor in the list or count() if no such descriptor is found.
        //!
        template<class DESC>
            requires std::derived_from<DESC, ts::AbstractDescriptor>
        size_t search(DuckContext& duck, DID tag, DESC& desc, size_t start_index = 0, PDS pds = 0) const;

        //!
        //! Total number of bytes that is required to serialize the list of descriptors.
        //! @param [in] start Starting index in the descriptor list.
        //! @param [in] count Maximum number of descriptors to include in the size.
        //! @return The total number of bytes that is required to serialize the list of descriptors.
        //!
        size_t binarySize(size_t start = 0, size_t count = NPOS) const;

        //!
        //! Serialize the content of the descriptor list.
        //! @param [in,out] addr Address of the memory area where the descriptors
        //! are serialized. Upon return, @a addr is updated to contain the next
        //! address in memory, after the last serialized byte.
        //! @param [in,out] size Size in bytes of the memory area where the descriptors
        //! are serialized. Upon return, @a size is updated to the remaining size
        //! of the buffer. Descriptors are written one by one until either the end
        //! of the list or until one descriptor does not fit.
        //! @param [in] start Start searializing at this index.
        //! @return The index of the first descriptor that could not be serialized
        //! (or count() if all descriptors were serialized). In the first case,
        //! the returned index can be used as @a start parameter to serialized the
        //! rest of the list (in another section for instance).
        //!
        size_t serialize(uint8_t*& addr, size_t& size, size_t start = 0) const;

        //!
        //! Serialize the content of the descriptor list in a byte block.
        //! @param [in,out] bb A byte block into which the descriptor list is appended.
        //! @param [in] start Start searializing at this index.
        //! @return The size in bytes of the serialized data.
        //!
        size_t serialize(ByteBlock& bb, size_t start = 0) const;

        //!
        //! Same as serialize(), but prepend a 2-byte length field before the descriptor list.
        //! The 2-byte length field has 4 reserved bits and 12 bits for the length of the descriptor list.
        //! In fact, the number of bits in the length can be set in @a length_bits.
        //! @param [in,out] addr Address of the memory area where the descriptors
        //! are serialized. Upon return, @a addr is updated to contain the next
        //! address in memory, after the last serialized byte.
        //! @param [in,out] size Size in bytes of the memory area where the descriptors
        //! are serialized. Upon return, @a size is updated to the remaining size
        //! of the buffer. Descriptors are written one by one until either the end
        //! of the list or until one descriptor does not fit.
        //! @param [in] start Start serializing at this index in the descriptor list.
        //! @param [in] reserved_bits Value of the upper bits of the length field.
        //! @param [in] length_bits Number of meaningful bits in the length field.
        //! @return The index of the first descriptor that could not be serialized
        //! (or count() if all descriptors were serialized). In the first case,
        //! the returned index can be used as @a start parameter to serialized the
        //! rest of the list (in another section for instance).
        //!
        size_t lengthSerialize(uint8_t*& addr, size_t& size, size_t start = 0, uint16_t reserved_bits = 0x000F, size_t length_bits = 12) const;

        //!
        //! This method converts a descriptor list to XML.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] parent The parent node for the XML descriptors.
        //! @return True on success, false on error.
        //!
        bool toXML(DuckContext& duck, xml::Element* parent) const;

        //!
        //! This method decodes an XML list of descriptors.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [out] others Returned list of non-descriptor XML elements.
        //! All these elements are not null and their names are in @a allowedOthers.
        //! @param [in] parent The XML element containing all descriptors.
        //! @param [in] allowedOthers A list of allowed element names inside @a parent which are not descriptors.
        //! @return True on success, false on error.
        //!
        bool fromXML(DuckContext& duck, xml::ElementVector& others, const xml::Element* parent, const UStringList& allowedOthers);

        //!
        //! This method decodes an XML list of descriptors.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [out] others Returned list of non-descriptor XML elements.
        //! All these elements are not null and their names are in @a allowedOthers.
        //! @param [in] parent The XML element containing all descriptors.
        //! @param [in] allowedOthers A comma-separated list of allowed element names inside @a parent which are not descriptors.
        //! @return True on success, false on error.
        //!
        bool fromXML(DuckContext& duck, xml::ElementVector& others, const xml::Element* parent, const UString& allowedOthers);

        //!
        //! This method decodes an XML list of descriptors.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] parent The XML element containing all descriptors. All children must be valid descriptors.
        //! @return True on success, false on error.
        //!
        bool fromXML(DuckContext& duck, const xml::Element* parent);

    private:
        // Vector of safe pointers to descriptors.
        std::vector<DescriptorPtr> _list {};

        // Add a descriptor with a 32-bit payload at end of list.
        void add32BitDescriptor(DID did, uint32_t payload);

        // Update a REGID or PDS value if the descriptor contains one.
        static void UpdateREGID(REGID& regid, const DescriptorPtr& desc);
        static void UpdatePDS(PDS& pds, const DescriptorPtr& desc);

        // Prepare removal of a private_data_specifier descriptor at the specified position, it any.
        // Return true if can be removed, false if it cannot (private descriptors ahead).
        bool canRemovePDS(std::vector<DescriptorPtr>::iterator);

        // Explore the descriptor and invoke a callback for each language which is found.
        // Use: bool callback(size_t descriptor_index, const char* lang, size_t lang_size)
        // The callback shall return true to continue, false to stop browsing languages in the descriptor list.
        template <typename F>
        void browseLanguages(const DuckContext& duck, size_t start_index, F callback) const;
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

// Search a descriptor with the specified tag, starting at the specified index.
template<class DESC> requires std::derived_from<DESC, ts::AbstractDescriptor>
size_t ts::DescriptorList::search(DuckContext& duck, DID tag, DESC& desc, size_t start_index, PDS pds) const
{
    // Repeatedly search for a descriptor until one is successfully deserialized
    for (size_t index = search(tag, start_index, pds); index < _list.size(); index = search(tag, index + 1, pds)) {
        if (_list[index] != nullptr) {
            desc.deserialize(duck, *(_list[index]));
            if (desc.isValid()) {
                return index;
            }
        }
    }

    // Not found
    desc.invalidate();
    return _list.size();
}
