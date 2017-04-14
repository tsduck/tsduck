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
//
//  List of MPEG PSI/SI descriptors
//
//----------------------------------------------------------------------------

#pragma once
#include "tsDescriptor.h"

namespace ts {

    class AbstractDescriptor;

    class TSDUCKDLL DescriptorList
    {
    public:

        // Default constructor: empty list
        DescriptorList () : _list () {}

        // Copy constructor: the descriptors objects are shared between the two lists.
        DescriptorList (const DescriptorList& dl) : _list (dl._list) {}

        // Return the number of descriptors in the list.
        size_t count() const {return _list.size ();}

        // Comparison
        bool operator== (const DescriptorList& other) const;
        bool operator!= (const DescriptorList& other) const {return !(*this == other);}

        // Return a reference to the descriptor at specified index.
        // Valid index are 0 to count()-1.
        const DescriptorPtr& operator[] (size_t index) const
        {
            assert (index < _list.size ());
            return _list[index].desc;
        }

        // Return the "private data specifier" associated to the
        // descriptor at the specified index.
        PDS privateDataSpecifier (size_t index) const
        {
            assert (index < _list.size ());
            return _list[index].pds;
        }

        // Add one descriptor at end of list
        void add (const DescriptorPtr&);
        void add (const AbstractDescriptor&);

        // Add another list of descriptors at end of list.
        // The descriptors objects are shared between the two lists.
        void add (const DescriptorList& dl)
        {
            _list.insert (_list.end(), dl._list.begin(), dl._list.end());
        }

        // Add descriptors from a memory area at end of list
        void add (const void*, size_t);

        // Add one descriptor from a memory area at end of list.
        // The size is extracted from the descriptor header.
        void add (const void* addr)
        {
            const uint8_t* data (reinterpret_cast <const uint8_t*> (addr));
            add (data, size_t (data[1]) + 2);
        }

        // Add a private_data_specifier if necessary at end of list
        void addPrivateDataSpecifier (PDS);

        // Remove the descriptor at the specified index in the list.
        // A private_data_specifier descriptor can be removed only if
        // it is not necessary (no private descriptor ahead).
        // Return true on success, false on error (index out of range
        // or required private_data_specifier descriptor).
        bool removeByIndex (size_t index);

        // Remove all descriptors with the specified tag.
        // If "pds" is non-zero and "tag" is >= 0x80, remove only
        // descriptors with the corresponding "private data specifier".
        // Return the number of removed descriptors.
        // A private_data_specifier descriptor can be removed only if
        // it is not necessary (no private descriptor ahead).
        size_t removeByTag (DID tag, PDS pds = 0);

        // Remove all private descriptors without preceding
        // private_data_specifier_descriptor.
        // Return the number of removed descriptors.
        size_t removeInvalidPrivateDescriptors();

        // Clear the content of the descriptor list.
        void clear()
        {
            _list.clear ();
        }

        // Search a descriptor with the specified tag, starting at the
        // specified index. Return the index of the descriptor in the list
        // or count() if no such descriptor is found. If "pds"
        // is non-zero and "tag" is >= 0x80, return only a descriptor with
        // the corresponding "private data specifier".
        size_t search (DID tag, size_t start_index = 0, PDS pds = 0) const;

        // Search a language descriptor for the specified language, starting at
        // the specified index. Return the index of the descriptor in the list
        // or count() if no such descriptor is found.
        size_t searchLanguage (const std::string& language, size_t start_index = 0) const;

        // Search any kind of subtitle descriptor, starting at the specified
        // index. Return the index of the descriptor in the list.
        // Return count() if no such descriptor is found.
        //
        // If the specified language is non-empty, look only for a subtitle
        // descriptor matching the specified language. In this case, if some
        // kind of subtitle descriptor exists in the list but none matches the
        // language, return count()+1.
        size_t searchSubtitle (const std::string& language = "", size_t start_index = 0) const;

        // Search a descriptor with the specified tag, starting at the
        // specified index. Return the index of the descriptor in the list
        // or count() if no such descriptor is found. If "pds"
        // is non-zero and "tag" is >= 0x80, return only a descriptor with
        // the corresponding "private data specifier".
        // The template class DESC is typically a subclass of AbstractDescriptor.
        // When a descriptor with the specified tag is found, it is deserialized
        // into desc. Always check desc.isValid() on return to check if the
        // deserialization was successful.
        template <class DESC>
        size_t search (DID tag, DESC& desc, size_t start_index = 0, PDS pds = 0) const;

        // Total number of bytes that is required to serialize the list of descriptors.
        size_t binarySize() const;

        // Serialize the content of the descriptor list.
        //
        // The descriptors are written in memory area addr/size.
        // Descriptors are written one by one until either the end
        // of the list or until one descriptor does not fit.
        //
        // Addr and size are updated to describe the remaining
        // free space, after the last serialized descriptor.
        //
        // Start at descriptor at index "start". Return the index
        // of the first descriptor that could not be serialized
        // (or count() if all descriptors were serialized).
        // In the first case, the returned index can be used
        // as "start" parameter to serialized the rest of the list
        // (in another section for instance).

        size_t serialize (uint8_t*& addr, size_t& size, size_t start = 0) const;

        // Same as serialize(), but prepend a 2-byte length field
        // before the descriptor list. The 2-byte length field
        // has 4 reserved bits (all '1') and 12 bits for the length
        // of the descriptor list.

        size_t lengthSerialize (uint8_t*& addr, size_t& size, size_t start = 0) const;

        // Display a descriptor list on an output stream
        // - indent : Line indentation for display
        // - tid    : Table id of table containing the descriptors. Optional.
        //            Used by some descriptors the interpretation of which may
        //            vary depending on the table that they are in.

        std::ostream& display (std::ostream& strm, int indent = 0, TID tid = TID_NULL) const;

    private:
        // Each entry contains a descriptor and its corresponding private data specifier.
        struct Element
        {
            // Public members:
            DescriptorPtr desc;
            PDS pds;

            // Constructor:
            Element (const DescriptorPtr& desc_ = 0, PDS pds_ = 0) : desc (desc_), pds (pds_) {}
        };
        typedef std::vector <Element> ElementVector;

        // Private members
        ElementVector _list;

        // Prepare removal of a private_data_specifier descriptor.
        // Return true if can be removed, false if it cannot (private descriptors ahead).
        // When it can be removed, the current PDS of all subsequent descriptors is updated.
        bool prepareRemovePDS (const ElementVector::iterator&);
    };
}

// Display operator for descriptor lists
TSDUCKDLL inline std::ostream& operator<< (std::ostream& strm, const ts::DescriptorList& dlist)
{
    return dlist.display (strm);
}

#include "tsDescriptorListTemplate.h"
