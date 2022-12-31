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
//!  Representation of a telephone_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a telephone_descriptor
    //! @see ETSI EN 300 468, 6.2.42.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL TelephoneDescriptor : public AbstractDescriptor
    {
    public:
        // TelephoneDescriptor public members:
        bool    foreign_availability;     //!< Can be called from outside the country.
        uint8_t connection_type;          //!< 5 bits, connection type.
        UString country_prefix;           //!< Country prefix.
        UString international_area_code;  //!< International area code.
        UString operator_code;            //!< Operator code.
        UString national_area_code;       //!< National area code.
        UString core_number;              //!< Core number.

        // Maximum string sizes, based on sizes of length fields.
        static constexpr size_t MAX_COUNTRY_PREFIX_LENGTH          = 3;  //!< Maximum length of @a country_prefix.
        static constexpr size_t MAX_INTERNATIONAL_AREA_CODE_LENGTH = 7;  //!< Maximum length of @a international_area_code.
        static constexpr size_t MAX_OPERATOR_CODE_LENGTH           = 3;  //!< Maximum length of @a operator_code.
        static constexpr size_t MAX_NATIONAL_AREA_CODE_LENGTH      = 7;  //!< Maximum length of @a national_area_code.
        static constexpr size_t MAX_CORE_NUMBER_LENGTH             = 15; //!< Maximum length of @a core_number.

        //!
        //! Default constructor.
        //!
        TelephoneDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        TelephoneDescriptor(DuckContext& duck, const Descriptor& bin);

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
