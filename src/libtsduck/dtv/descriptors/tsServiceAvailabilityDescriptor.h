//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a service_availability_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a service_availability_descriptor
    //! @see ETSI EN 300 468, 6.2.34.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ServiceAvailabilityDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Maximum number of cells to fit in 255 bytes.
        //!
        static constexpr size_t MAX_CELLS = 127;

        // ServiceAvailabilityDescriptor public members:
        bool                  availability = false;  //!< The service is available/unavailable in the listed cells.
        std::vector<uint16_t> cell_ids {};           //!< The cell ids.

        //!
        //! Default constructor.
        //!
        ServiceAvailabilityDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ServiceAvailabilityDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer& buf) const override;
        virtual void deserializePayload(PSIBuffer& buf) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
