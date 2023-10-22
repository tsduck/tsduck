//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a country_availability_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a country_availability_descriptor.
    //! @see ETSI EN 300 468, 6.2.10.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL CountryAvailabilityDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        bool          country_availability = true; //!< See ETSI EN 300 468, 6.2.10.
        UStringVector country_codes {};            //!< See ETSI EN 300 468, 6.2.10.

        //!
        //! Maximum number of entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 84;

        //!
        //! Default constructor.
        //!
        CountryAvailabilityDescriptor();

        //!
        //! Constructor using a variable-length argument list.
        //! @param [in] availability If true, the service is available in the specified countries.
        //! @param [in] countries Variable-length list of country codes.
        //!
        CountryAvailabilityDescriptor(bool availability, const std::initializer_list<UString> countries);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        CountryAvailabilityDescriptor(DuckContext& duck, const Descriptor& bin);

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
