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
            UString language_code;  //!< ISO-639 language code, 3 characters.
            uint8_t audio_type;     //!< Audio type.

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
        EntryList entries;  //!< List of language entries.

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
