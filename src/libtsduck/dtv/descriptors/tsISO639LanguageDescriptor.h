//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISO_639_language_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ISO_639_language_descriptor
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.18.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ISO639LanguageDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Language entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            UString language_code {};  //!< ISO-639 language code, 3 characters.
            uint8_t audio_type = 0;    //!< Audio type.

            //!
            //! Default constructor.
            //! @param [in] code ISO-639 language code, 3 characters, as a C-string. Can be null.
            //! @param [in] type Audio type.
            //!
            Entry(const UChar* code = nullptr, uint8_t type = 0);

            //!
            //! Constructor.
            //! @param [in] code ISO-639 language code, 3 characters, as a C++ string.
            //! @param [in] type Audio type.
            //!
            Entry(const UString& code, uint8_t type);

            //!
            //! Get a string representing the audio type.
            //! @param [in] flags Presentation flags.
            //! @return A string representing the audio type.
            //!
            UString audioTypeName(NamesFlags flags = NamesFlags::NAME) const;
        };

        //!
        //! List of language entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of language entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 63;

        // ISO639LanguageDescriptor public members:
        EntryList entries {};  //!< List of language entries.

        //!
        //! Default constructor.
        //!
        ISO639LanguageDescriptor();

        //!
        //! Constructor with one language code.
        //! @param [in] code ISO-639 language code, 3 characters.
        //! @param [in] type Audio type.
        //!
        ISO639LanguageDescriptor(const UString& code, uint8_t type);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ISO639LanguageDescriptor(DuckContext& duck, const Descriptor& bin);

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
