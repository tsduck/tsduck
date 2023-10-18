//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB service_group_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {

    //!
    //! Representation of an ISDB service_group_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.49
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ServiceGroupDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Simultaneous service entry.
        //!
        struct TSDUCKDLL SimultaneousService
        {
            SimultaneousService() = default;    //!< Constructor.
            uint16_t primary_service_id = 0;    //!< Primary service id.
            uint16_t secondary_service_id = 0;  //!< Secondary service id.
        };

        //!
        //!< List of simultaneous service entries.
        //!
        typedef std::list<SimultaneousService> SimultaneousServiceList;

        // ServiceGroupDescriptor public members:
        uint8_t                 service_group_type = 0;    //!< 4 bits, group type.
        SimultaneousServiceList simultaneous_services {};  //!< List of simultaneous service, when service_group_type == 1.
        ByteBlock               private_data {};           //!< Private data for other group types.

        //!
        //! Default constructor.
        //!
        ServiceGroupDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ServiceGroupDescriptor(DuckContext& duck, const Descriptor& bin);

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
