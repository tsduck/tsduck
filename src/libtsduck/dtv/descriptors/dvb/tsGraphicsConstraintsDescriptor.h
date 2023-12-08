//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a graphics_constraints_descriptor (AIT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a graphics_constraints_descriptor (AIT specific).
    //!
    //! This descriptor cannot be present in other tables than an AIT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI TS 102 809, 5.3.5.8.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL GraphicsConstraintsDescriptor : public AbstractDescriptor
    {
    public:
        // GraphicsConstraintsDescriptor public members:
        bool      can_run_without_visible_ui = false;           //!< Can run without visible UI.
        bool      handles_configuration_changed = false;        //!< Handles configuration changed.
        bool      handles_externally_controlled_video = false;  //!< Handles externally controlled video.
        ByteBlock graphics_configuration {};                    //!< Graphics configuration bytes.

        //!
        //! Default constructor.
        //!
        GraphicsConstraintsDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        GraphicsConstraintsDescriptor(DuckContext& duck, const Descriptor& bin);

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
