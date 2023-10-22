//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a transport_stream_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsUString.h"

namespace ts {
    //!
    //! Representation of a transport_stream_descriptor
    //! @see ETSI EN 300 468, 6.2.46.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL TransportStreamDescriptor : public AbstractDescriptor
    {
    public:
        // TransportStreamDescriptor public members:
        UString compliance {}; //!< Standard compliance ("DVB" for DVB systems).

        //!
        //! Default constructor.
        //! @param [in] comp Compliance name.
        //!
        TransportStreamDescriptor(const UString& comp = UString());

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        TransportStreamDescriptor(DuckContext& duck, const Descriptor& bin);

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
