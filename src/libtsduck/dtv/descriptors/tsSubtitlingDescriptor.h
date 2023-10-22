//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a subtitling_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a subtitling_descriptor.
    //! @see ETSI EN 300 468, 6.2.41.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SubtitlingDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! An item entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            UString  language_code {};          //!< ISO-639 language code, 3 characters.
            uint8_t  subtitling_type = 0;       //!< Subtitling type.
            uint16_t composition_page_id = 0;   //!< Composition page identifier.
            uint16_t ancillary_page_id = 0;     //!< Ancillary page identifier.

            //!
            //! Default constructor.
            //! @param [in] code ISO-639 language code, 3 characters. Can be null.
            //! @param [in] subt Subtitling type.
            //! @param [in] comp Composition page identifier.
            //! @param [in] ancil Ancillary page identifier.
            //!
            Entry(const UChar* code = nullptr, uint8_t subt = 0, uint16_t comp = 0, uint16_t ancil = 0);

            //!
            //! Default constructor.
            //! @param [in] code ISO-639 language code, 3 characters.
            //! @param [in] subt Subtitling type.
            //! @param [in] comp Composition page identifier.
            //! @param [in] ancil Ancillary page identifier.
            //!
            Entry(const UString& code, uint8_t subt = 0, uint16_t comp = 0, uint16_t ancil = 0);

            //!
            //! Get the name of the subtitling type.
            //! @return The name of the subtitling type.
            //!
            UString subtitlingTypeName() const;
        };

        //!
        //! List of language entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of language entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 31;

        // Public members
        EntryList entries {};  //!< The list of item entries in the descriptor.

        //!
        //! Default constructor.
        //!
        SubtitlingDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SubtitlingDescriptor(DuckContext& duck, const Descriptor& bin);

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
