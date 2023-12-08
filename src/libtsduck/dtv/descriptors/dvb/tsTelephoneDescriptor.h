//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        bool    foreign_availability = false;  //!< Can be called from outside the country.
        uint8_t connection_type = 0;           //!< 5 bits, connection type.
        UString country_prefix {};             //!< Country prefix.
        UString international_area_code {};    //!< International area code.
        UString operator_code {};              //!< Operator code.
        UString national_area_code {};         //!< National area code.
        UString core_number {};                //!< Core number.

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
