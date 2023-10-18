//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
            uint8_t  teletext_type = 0;  //!< Teletext type, 5 bits.
            uint16_t page_number = 0;    //!< Teletext page number, combination of page and magazine number.
            UString  language_code {};   //!< ISO-639 language code, 3 characters.

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
        static constexpr size_t MAX_ENTRIES = 51;

        // Public members
        EntryList entries {};  //!< The list of item entries in the descriptor.

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
