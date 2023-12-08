//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a registration_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a registration_descriptor
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.8.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL RegistrationDescriptor : public AbstractDescriptor
    {
    public:
        // RegistrationDescriptor public members:
        uint32_t  format_identifier = 0;              //!< Identifier obtained from a Registration Authority.
        ByteBlock additional_identification_info {};  //!< Identifier-dependent information.

        //!
        //! Default constructor.
        //! @param [in] identifier Identifier obtained from a Registration Authority.
        //! @param [in] info Identifier-dependent information.
        //!
        RegistrationDescriptor(uint32_t identifier = 0, const ByteBlock& info = ByteBlock());

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        RegistrationDescriptor(DuckContext& duck, const Descriptor& bin);

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
