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
//  Representation of an ISO_639_language_descriptor
//
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {

    class TSDUCKDLL ISO639LanguageDescriptor : public AbstractDescriptor
    {
    public:
        // Language entry
        struct TSDUCKDLL Entry
        {
            // Public members
            std::string language_code;  // 3 characters
            uint8_t       audio_type;

            // Contructors
            Entry (const char* code = 0, uint8_t type = 0): language_code (code != 0 ? code : ""), audio_type (type) {}
            Entry (const std::string& code, uint8_t type): language_code (code), audio_type (type) {}
        };
        typedef std::list<Entry> EntryList;
            
        // Maximum number of entries to fit in 255 bytes
        static const size_t MAX_ENTRIES = 63;

        // ISO639LanguageDescriptor public members:
        EntryList entries;

        // Default constructor:
        ISO639LanguageDescriptor();

        // Constructor with one language code.
        ISO639LanguageDescriptor (const std::string& code, uint8_t type);

        // Constructor from a binary descriptor
        ISO639LanguageDescriptor (const Descriptor&);

        // Inherited methods
        virtual void serialize (Descriptor&) const;
        virtual void deserialize (const Descriptor&);
    };
}
