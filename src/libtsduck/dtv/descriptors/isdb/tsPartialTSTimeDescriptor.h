//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB partialTS_time_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an ISDB partialTS_time_descriptor.
    //! @see ARIB STD-B21, 9.1.8.3 (3)
    //! @ingroup libtsduck descriptor
    //!
    class TSDUCKDLL PartialTSTimeDescriptor : public AbstractDescriptor
    {
    public:
        // PartialTSTimeDescriptor public members:
        uint8_t                    event_version_number = 0;         //!< Event version number.
        std::optional<Time>        event_start_time {};              //!< Event start time.
        std::optional<cn::seconds> duration {};                      //!< Event duration.
        std::optional<cn::seconds> offset {};                        //!< Event offset, positive or negative.
        bool                       other_descriptor_status = false;  //!< Other descriptors have not changed / may have changed.
        std::optional<Time>        JST_time {};                      //!< Current JST time.

        //!
        //! Default constructor.
        //!
        PartialTSTimeDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        PartialTSTimeDescriptor(DuckContext& duck, const Descriptor& bin);

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
