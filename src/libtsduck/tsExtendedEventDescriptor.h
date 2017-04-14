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
//  Representation of an extended_event_descriptor
//
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsDescriptorList.h"

namespace ts {

    class TSDUCKDLL ExtendedEventDescriptor : public AbstractDescriptor
    {
    public:
        // Item entry
        struct Entry;
        typedef std::list<Entry> EntryList;

        // Public members
        uint8_t       descriptor_number;
        uint8_t       last_descriptor_number;
        std::string language_code;  // 3 characters
        EntryList   entries;
        std::string text;

        // Default constructor:
        ExtendedEventDescriptor();

        // Constructor from a binary descriptor
        ExtendedEventDescriptor (const Descriptor&);

        // Split the content into several ExtendedEventDescriptor if the content
        // is too long and add them in a descriptor list.
        void splitAndAdd (DescriptorList&) const;

        // Inherited methods
        virtual void serialize (Descriptor&) const;
        virtual void deserialize (const Descriptor&);

        // Normalize all ExtendedEventDescriptor in a descriptor list.
        // Update all descriptor_number and last_descriptor_number per language.
        static void NormalizeNumbering (uint8_t* desc_list_addr, size_t desc_list_size);

        // Item entry
        struct TSDUCKDLL Entry
        {
            // Public members
            std::string item_description;
            std::string item;

            // Contructors
            Entry (const std::string& desc_ = "", const std::string& item_ = "") : item_description (desc_), item (item_) {}
            Entry (const Entry& other) : item_description (other.item_description), item (other.item) {}
        };
    };
}
