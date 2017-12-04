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
    //! @see ETSI 300 468, 6.2.43.
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
            Entry(const UChar* code = 0, uint8_t type = 0, uint16_t page = 0);

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
            //! @return Full page number.
            //!
            void setFullNumber(uint8_t teletext_magazine_number, uint8_t teletext_page_number);

            //!
            //! Extract magazine number from the full Teletext page number.
            //! @return Teletext magazine number, 3-bit value from descriptor.
            //! @see setFullNumber()
            //!
            uint8_t pageNumber() const;

            //!
            //! Extract page number from then full Teletext page number.
            //! @return Teletext page number, 8-bit value from descriptor.
            //! @see setFullNumber()
            //!
            uint8_t magazineNumber() const;
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
        //! @param [in] bin A binary descriptor to deserialize.
        //! @param [in] charset If not zero, character set to use without explicit table code.
        //!
        TeletextDescriptor(const Descriptor& bin, const DVBCharset* charset = 0);

        // Inherited methods
        virtual void serialize(Descriptor&, const DVBCharset* = 0) const override;
        virtual void deserialize(const Descriptor&, const DVBCharset* = 0) override;
        virtual XML::Element* toXML(XML&, XML::Element*) const override;
        virtual void fromXML(XML&, const XML::Element*) override;

        //!
        //! Static method to display a descriptor.
        //! @param [in,out] display Display engine.
        //! @param [in] did Descriptor id.
        //! @param [in] payload Address of the descriptor payload.
        //! @param [in] size Size in bytes of the descriptor payload.
        //! @param [in] indent Indentation width.
        //! @param [in] tid Table id of table containing the descriptors.
        //! @param [in] pds Private Data Specifier. Used to interpret private descriptors.
        //!
        static void DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* payload, size_t size, int indent, TID tid, PDS pds);
    };
}
