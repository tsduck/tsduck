//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
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
//!  Representation of an Media_service_kind_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsVariable.h"

namespace ts {
    //!
    //! Representation of an Media_service_kind_descriptor.
    //!
    //! @see ISO/IEC 13818-1 (Amd.1) 2.6.141
    //! @ingroup descriptor
    //!
    //! The media_service_kind_descriptor can read on the Program or PID ES stream itself or can
    //! read on auxiliary PID ES that are dependent on the principal PID media component ES (i.e.
    //! in most cases the principal media component is video). The media service kind descriptor
    //! can also read upon the entire program to describe content with a single principal media
    //! component. Multiple data elements (N) can be included in the descriptor each with one or
    //! more language code/media_service_type pairs.
    //! If the same media component is described at a Program Leveland an Elementary Level, then
    //! the Elementary stream media_service_kind description shall take precedence.
    //!
    class TSDUCKDLL MediaServiceKindDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! An association between a language and a service type.
        //!
        class language_media_pair_type {
        public:
            uint8_t              configuration_type;     //!< 2 bits
            uint8_t              lang_len_idc;           //!< 2 bits
            uint8_t              lang_len;               //!< 8 bits, only used if lang_len_idc==7
            UString              language_code;          //!< BCP47 compliant language code
            std::vector<uint8_t> media_service_types;    //!< service types associated with this language

            language_media_pair_type();        //!< Constructor
        };

        //!
        //! Description of a media service.
        //!
        class media_service_kind_type {
        public:
            uint8_t                                 media_description_flag;  //!< 1 bit
            uint8_t                                 media_type_idc;          //!< 2 bits
            Variable<uint8_t>                       ID_length_code;          //!< 3 bits
            Variable<uint16_t>                      ID_type;                 //!< 13 bits
            uint8_t                                 ID_len;                  //!< 8 bits, only used if ID_length_code==0
            UString                                 media_ID_field;          //!< identifier of the data element
            std::vector<language_media_pair_type>   language_media_service_type_pairs;  //!< list of language/service type pairs for this media type

            media_service_kind_type();          //!< Constructor
        };

        // Public members:
        std::vector<media_service_kind_type>     media_service_kinds;  //!< Description of all media services.

        //!
        //! Default constructor.
        //!
        MediaServiceKindDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        MediaServiceKindDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    private:
        // Enumerations for XML.
        static const Enumeration MediaDescriptionFlag;
        static const Enumeration MediaType;

    protected:
        // Inherited methods
        virtual DID extendedTag() const override;
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
