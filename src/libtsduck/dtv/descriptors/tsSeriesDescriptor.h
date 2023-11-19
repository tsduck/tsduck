//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB series_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an ISDB series_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.33
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SeriesDescriptor : public AbstractDescriptor
    {
    public:
        // SeriesDescriptor public members:
        uint16_t            series_id = 0;            //!< Series id.
        uint8_t             repeat_label = 0;         //!< 4 bits.
        uint8_t             program_pattern = 0;      //!< 3 bits.
        std::optional<Time> expire_date {};           //!< Optional expiration date (the time inside the day is ignored).
        uint16_t            episode_number = 0;       //!< 12 bits.
        uint16_t            last_episode_number = 0;  //!< 12 bits.
        UString             series_name {};           //!< Series name.

        //!
        //! Default constructor.
        //!
        SeriesDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SeriesDescriptor(DuckContext& duck, const Descriptor& bin);

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
