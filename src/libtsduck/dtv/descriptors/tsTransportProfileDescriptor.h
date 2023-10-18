//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a transport_profile_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a transport_profile_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.93.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL TransportProfileDescriptor : public AbstractDescriptor
    {
    public:
        // TransportProfileDescriptor public members:
        uint8_t   transport_profile = 0;  //!< Transport profile.
        ByteBlock private_data {};        //!< Private data.

        //!
        //! Default constructor.
        //!
        TransportProfileDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        TransportProfileDescriptor(DuckContext& duck, const Descriptor& bin);

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
