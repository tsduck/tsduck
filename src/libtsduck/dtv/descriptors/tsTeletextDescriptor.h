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
//!  Representation of a teletext_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a teletext_descriptor.
    //! @see ETSI EN 300 468, 6.2.43.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL TeletextDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! An item entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            uint8_t  teletext_type;  //!< Teletext type, 5 bits.
            uint16_t page_number;    //!< Teletext page number, combination of page and magazine number.
            UString  language_code;  //!< ISO-639 language code, 3 characters.

            //!
            //! Default constructor.
            //! @param [in] code ISO-639 language code, 3 characters. Can be null.
            //! @param [in] type Teletext type, 5 bits.
            //! @param [in] page Teletext page number, combination of page and magazine number.
            //!
            Entry(const UChar* code = nullptr, uint8_t type = 0, uint16_t page = 0);

            //!
            //! Default constructor.
            //! @param [in] type Teletext type, 5 bits.
            //! @param [in] page Teletext page number, combination of page and magazine number.
            //! @param [in] code ISO-639 language code, 3 characters.
            //!
            Entry(const UString& code, uint8_t type = 0, uint16_t page = 0);

            //!
            //! Build a full Teletext page number from magazine and page numbers.
            //! In Teletext, a "page number" is built from two data, the magazine and page numbers.
            //! The binary descriptor contains these two values.
            //! @param [in] teletext_magazine_number Teletext magazine number, 3-bit value from descriptor.
            //! @param [in] teletext_page_number Teletext page number, 8-bit value from descriptor.
            //!
            void setFullNumber(uint8_t teletext_magazine_number, uint8_t teletext_page_number);

            //!
            //! Extract magazine number from the full Teletext page number.
            //! @return Teletext magazine number, 3-bit value from descriptor.
            //! @see setFullNumber()
            //!
            uint8_t magazineNumber() const;

            //!
            //! Extract page number from then full Teletext page number.
            //! @return Teletext page number, 8-bit value from descriptor.
            //! @see setFullNumber()
            //!
            uint8_t pageNumber() const;
        };

        //!
        //! List of language entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of language entries to fit in 255 bytes.
        //!
        static const size_t MAX_ENTRIES = 51;

        // Public members
        EntryList entries;  //!< The list of item entries in the descriptor.

        //!
        //! Default constructor.
        //!
        TeletextDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        TeletextDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        //!
        //! Protected constructor for subclasses.
        //! This is used by subclasses which have exactly the same structure as a teletext_descriptor.
        //! @param [in] tag Descriptor tag.
        //! @param [in] xml_name Descriptor name, as used in XML structures.
        //! @param [in] standards A bit mask of standards which define this structure.
        //! @param [in] pds Required private data specifier if this is a private descriptor.
        //!
        TeletextDescriptor(DID tag, const UChar* xml_name, Standards standards, PDS pds);

        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
