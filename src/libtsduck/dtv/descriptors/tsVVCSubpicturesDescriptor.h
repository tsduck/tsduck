//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DVB VVC_subpictures_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a DVB VVC_subpictures_descriptor.
    //! @see ETSI EN 300 468, clause 6.4.17.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL VVCSubpicturesDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        bool                 default_service_mode = 0;  //!< EN 300 468 clause 6.4.17
        std::vector<uint8_t> component_tag {};          //!< 8 bits, EN 300 468 clause 6.4.17
        std::vector<uint8_t> vvc_subpicture_id {};      //!< 8 bits, EN 300 468 clause 6.4.17
        uint8_t              processing_mode = 0;       //!< 3 bits, EN 300 468 clause 6.4.17
        UString              service_description {};    //!< EN 300 468 clause 6.4.17

        //!
        //! Default constructor.
        //!
        VVCSubpicturesDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        VVCSubpicturesDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

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
