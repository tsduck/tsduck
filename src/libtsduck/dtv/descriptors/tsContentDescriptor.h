//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  Representation of a content_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a content_descriptor.
    //! @see ETSI EN 300 468, 6.2.9.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ContentDescriptor : public AbstractDescriptor
    {
    public:
        // Content entry
        struct Entry;

        //!
        //! A list of content entries.
        //!
        typedef std::list<Entry> EntryList;

        // Public members:
        EntryList entries;  //!< The list of content entries.

        //!
        //! Maximum number of entries to fit in 255 bytes.
        //!
        static const size_t MAX_ENTRIES = 127;

        //!
        //! Default constructor.
        //!
        ContentDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ContentDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! A content entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            uint8_t content_nibble_level_1;  //!< 4 bits, see ETSI EN 300 468, 6.2.9.
            uint8_t content_nibble_level_2;  //!< 4 bits, see ETSI EN 300 468, 6.2.9.
            uint8_t user_nibble_1;           //!< 4 bits, see ETSI EN 300 468, 6.2.9.
            uint8_t user_nibble_2;           //!< 4 bits, see ETSI EN 300 468, 6.2.9.

            //!
            //! Default constructor.
            //! @param [in] all All 4 nibbles as a 16-bit integer.
            //!
            Entry(uint16_t all = 0) :
                content_nibble_level_1(uint8_t(all >> 12) & 0x0F),
                content_nibble_level_2(uint8_t(all >> 8) & 0x0F),
                user_nibble_1(uint8_t(all >> 4) & 0x0F),
                user_nibble_2(uint8_t(all) & 0x0F)
            {
            }
            //!
            //! Constructor.
            //! @param [in] l1 Level 1 nibble.
            //! @param [in] l2 Level 2 nibble.
            //! @param [in] u1 First user nibble.
            //! @param [in] u2 Second user nibble.
            //!
            Entry (uint8_t l1, uint8_t l2, uint8_t u1, uint8_t u2) :
                content_nibble_level_1 (l1),
                content_nibble_level_2 (l2),
                user_nibble_1 (u1),
                user_nibble_2 (u2)
            {
            }
        };

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
