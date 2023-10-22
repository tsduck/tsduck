//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an parental_rating_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an parental_rating_descriptor
    //! @see ETSI EN 300 468, 6.2.28.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ParentalRatingDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Item entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            UString country_code {};  //!< ISO-3166 country code, 3 characters.
            uint8_t rating = 0;       //!< Parental rating.

            //!
            //! Constructor.
            //! @param [in] code ISO-3166 country code, 3 characters, as a C-string. Can be null.
            //! @param [in] rate Parental rating.
            //!
            Entry(const UChar* code = nullptr, uint8_t rate = 0);

            //!
            //! Constructor.
            //! @param [in] code ISO-3166 country code, 3 characters.
            //! @param [in] rate Parental rating.
            //!
            Entry(const UString& code, uint8_t rate);
        };

        //!
        //! A list of item entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of services entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 63;

        // Public members
        EntryList entries {};  //!< The list of item entries.

        //!
        //! Default constructor.
        //!
        ParentalRatingDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ParentalRatingDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Constructor with one entry.
        //! @param [in] code ISO-3166 country code, 3 characters.
        //! @param [in] rate Parental rating.
        //!
        ParentalRatingDescriptor(const UString& code, uint8_t rate);

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
